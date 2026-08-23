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
static bool stationConnectedOnce = false; // mark first successful STA connect
static unsigned long apIdleSince = 0;     // millis the AP last had zero clients
static bool apDisabled = false;           // AP turned off to save power

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

        stationConnectedOnce = true;
        apIdleSince = millis();          // start the AP power-down countdown

        // If the AP was powered down earlier, the station must have dropped and
        // recovered; AP is re-enabled by the disconnect handler, nothing to do.
    }, ARDUINO_EVENT_WIFI_STA_GOT_IP);

    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        Serial.println("STA disconnected, will auto-reconnect.");
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

    // Setup AP and STA
    APMode();
    WIFIC_setupCallbacks();
    WIFIC_stationMode();
}

// -----------------------------------------------------------------------------
// Periodic processing: drop the AP once the station is up and no AP clients
// have been connected for AP_AUTO_OFF_MS (saves power in station-only mode).
// -----------------------------------------------------------------------------
void WIFIC_process(void) {
    if (apDisabled || !stationConnectedOnce || WiFi.status() != WL_CONNECTED) {
        return;
    }

    if (WiFi.softAPgetStationNum() > 0) {
        apIdleSince = millis();          // clients present, keep the AP up
        return;
    }

    if ((millis() - apIdleSince) > AP_AUTO_OFF_MS) {
        Serial.println("No AP clients; disabling AP to save power.");
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        apDisabled = true;
    }
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
String WIFIC_getApList(void) {
    int n = WiFi.scanComplete();
    if (n <= 0) {                        // -1 running, -2 failed, 0 none
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

