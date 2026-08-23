/*
 *  Author: Rada Berar
 *  Email: ujagaga@gmail.com
 *
 *  Simplified WiFi connection module for ESP32:
 *  - AP always on
 *  - STA connects if saved
 *  - Automatic reconnection handled by ESP32 core
 *  - EEPROM stores SSID and password
 */

#include <WiFi.h>
#include <EEPROM.h>
#include "config.h"
#include "NTPSync.h"

// -----------------------------------------------------------------------------
// Local variables
// -----------------------------------------------------------------------------
static char myApName[32] = {0};         // AP name
static char st_ssid[SSID_SIZE] = {0};   // Saved SSID
static char st_pass[WIFI_PASS_SIZE] = {0}; // Saved password
static IPAddress stationIP;
static IPAddress apIP(192, 168, 4, 1);
static unsigned long bootMillis = 0;      // set once in WIFIC_init, for the AP grace period
static bool apDisabled = false;           // AP turned off to save power
static bool staStarted = false;           // first STA connect attempt made yet

// -----------------------------------------------------------------------------
// Getters
// -----------------------------------------------------------------------------
char* WIFIC_getDeviceName(void) {
    return myApName;
}

String WIFIC_getApIp(void) {
    return apIP.toString();
}

String WIFIC_getStSSID(void) {
    return String(st_ssid);
}

String WIFIC_getStPass(void) {
    return String(st_pass);
}

// -----------------------------------------------------------------------------
// EEPROM helpers
// -----------------------------------------------------------------------------
void WIFIC_setStSSID(String new_ssid) {
    EEPROM.begin(EEPROM_SIZE);
    uint16_t addr = 0;
    for (; addr < new_ssid.length() && addr < SSID_SIZE - 1; addr++) {
        EEPROM.put(addr + SSID_EEPROM_ADDR, new_ssid[addr]);
        st_ssid[addr] = new_ssid[addr];
    }
    EEPROM.put(addr + SSID_EEPROM_ADDR, 0);
    st_ssid[addr] = 0;
    EEPROM.commit();
}

void WIFIC_setStPass(String new_pass) {
    EEPROM.begin(EEPROM_SIZE);
    uint16_t addr = 0;
    for (; addr < new_pass.length() && addr < WIFI_PASS_SIZE - 1; addr++) {
        EEPROM.put(addr + WIFI_PASS_EEPROM_ADDR, new_pass[addr]);
        st_pass[addr] = new_pass[addr];
    }
    EEPROM.put(addr + WIFI_PASS_EEPROM_ADDR, 0);
    st_pass[addr] = 0;
    EEPROM.commit();
}

// -----------------------------------------------------------------------------
// Wi-Fi AP mode
// -----------------------------------------------------------------------------
static void APMode(void) {
    Serial.println("\nStarting AP");

    WiFi.mode(WIFI_AP_STA);          // Ensure AP+STA mode
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);

    String apName = String(AP_NAME_PREFIX) + WiFi.macAddress();
    apName.toCharArray(myApName, 16);

    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(myApName, AP_PASS);

    Serial.printf("AP active: %s, IP: %s\n", myApName, apIP.toString().c_str());
}

// -----------------------------------------------------------------------------
// Wi-Fi STA mode
// -----------------------------------------------------------------------------
void WIFIC_stationMode(void) {
    staStarted = true;   // also covers the delayed auto-trigger in WIFIC_process
    if (st_ssid[0] == 0) {
        Serial.println("No saved WiFi credentials.");
        return;
    }

    Serial.printf("Connecting STA [%s]...\n", st_ssid);
    WiFi.begin(st_ssid, st_pass);
}

// -----------------------------------------------------------------------------
// Wi-Fi event callbacks
// -----------------------------------------------------------------------------
void WIFIC_setupCallbacks(void) {
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        stationIP = WiFi.localIP();
        Serial.printf("\n\nConnected, IP: %s\n", stationIP.toString().c_str());
    }, ARDUINO_EVENT_WIFI_STA_GOT_IP);

    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        Serial.printf("STA disconnected, reason=%d, will auto-reconnect.\n", info.wifi_sta_disconnected.reason);
        if (apDisabled) {
            Serial.println("Re-enabling AP (station lost).");
            APMode();                    // bring the AP back so the device stays reachable
            apDisabled = false;
        }
    }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
}

// -----------------------------------------------------------------------------
// Initialize Wi-Fi module
// -----------------------------------------------------------------------------
void WIFIC_init(void) {
    bootMillis = millis();
    EEPROM.begin(EEPROM_SIZE);

    // Load saved password
    uint16_t i = 0;
    do {
        EEPROM.get(i + WIFI_PASS_EEPROM_ADDR, st_pass[i]);
        if ((st_pass[i] < 32) || (st_pass[i] > 126)) break;
        i++;
    } while (i < WIFI_PASS_SIZE);
    st_pass[i] = 0;

    // Load saved SSID
    i = 0;
    do {
        EEPROM.get(i + SSID_EEPROM_ADDR, st_ssid[i]);
        if ((st_ssid[i] < 32) || (st_ssid[i] > 126)) break;
        i++;
    } while (i < SSID_SIZE);
    st_ssid[i] = 0;

    // Setup AP; the first STA connect attempt is delayed (see WIFIC_process)
    // so it doesn't compete with the setup/scan window for the radio.
    APMode();
    WIFIC_setupCallbacks();
}

// -----------------------------------------------------------------------------
// Periodic processing:
// - The first STA connect attempt waits until STA_CONNECT_DELAY_MS after boot,
//   so the config page's WiFi scans have the single shared radio to themselves
//   during setup (a concurrent STA connect/retry cycle starves scans of airtime).
// - The AP stays up for at least AP_AUTO_OFF_MS after boot (grace period to
//   reconfigure) regardless of station state; after that, it's only torn down
//   once zero clients are connected to it -- never kicks an active client.
// -----------------------------------------------------------------------------
void WIFIC_process(void) {
    if (!staStarted && (millis() - bootMillis) >= STA_CONNECT_DELAY_MS) {
        WIFIC_stationMode();
    }

    if (apDisabled) {
        return;
    }

    if ((millis() - bootMillis) < AP_AUTO_OFF_MS) {
        return;
    }

    if (WiFi.softAPgetStationNum() > 0) {
        return;                          // client present, keep the AP up
    }

    Serial.println("No AP clients after grace period; disabling AP to save power.");
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    apDisabled = true;
}

// -----------------------------------------------------------------------------
// Return list of scanned APs
// -----------------------------------------------------------------------------
// Kick off a non-blocking scan, read results later so the HTTP handler never
// blocks. The page polls WIFIC_getApList() via /aplist.
void WIFIC_startScan(void) {
    WiFi.scanDelete();
    WiFi.scanNetworks(true);             // async
}

// Return the completed scan as a '|'-joined list, or "" if it is still running.
// The radio can be too busy to even start a scan (e.g. mid STA reconnect); in
// that case retry starting it so a later poll gets a real chance to complete.
String WIFIC_getApList(void) {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED) {
        WiFi.scanNetworks(true);
        return "";
    }
    if (n <= 0) {                         // -1 still running, 0 no networks found
        return "";
    }
    String result = WiFi.SSID(0);
    for (int i = 1; i < n; ++i) {
        result += "|" + WiFi.SSID(i);
    }
    WiFi.scanDelete();                   // free the result buffer
    return result;
}

String WIFIC_getStationIp()
{
    if (WiFi.status() == WL_CONNECTED){
        return WiFi.localIP().toString();
    }
    return "";
}

bool WIFIC_stationConnected()
{
    return (WiFi.status() == WL_CONNECTED);
}

