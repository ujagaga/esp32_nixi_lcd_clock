#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Timezone.h>

// ---- NTP / WiFi config ----
static const char* NTP_SERVERS[] = {
"rs.pool.ntp.org",
"time.cloudflare.com",
"time.google.com"
};
static const unsigned long NTP_UPDATE_INTERVAL_MS = 10 * 60 * 1000; // 10min

// ---- NTPClient + Timezone setup ----
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVERS[0], 0, NTP_UPDATE_INTERVAL_MS);

// ---- Serbian Timezone rules ----
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120}; // UTC+2 summer
TimeChangeRule CET = {"CET", Last, Sun, Oct, 3, 60}; // UTC+1 winter
Timezone serbiaTZ(CEST, CET);

static bool ntpSynced = false;

void NTPS_init() {
  ntpSynced = false;
  if (WiFi.status() != WL_CONNECTED) {
      Serial.println("NTPS: WiFi not connected yet, waiting...");
      return;
  }
  Serial.println("NTPS: starting NTP client...");
  timeClient.begin();
  timeClient.update();  
}

void NTPS_process() {
  if (WiFi.status() != WL_CONNECTED) return;

  // The library handles the interval internally based on NTP_UPDATE_INTERVAL_MS
  // Don't use forceUpdate() in a loop; it bypasses the safety timers.
  if (timeClient.update()) {
      if (timeClient.getEpochTime() > 100000) {    
          if (!ntpSynced) {
              Serial.println("NTPS: Time Synced Successfully");
              ntpSynced = true;
          }
      }
  }
}

bool NTPS_hasSynced() {
  return ntpSynced;
}

time_t NTPS_getLocalEpoch() {
  time_t utc = timeClient.getEpochTime();
  return serbiaTZ.toLocal(utc);
}

String NTPS_getHH() {
  time_t local = NTPS_getLocalEpoch();
  struct tm t;
  localtime_r(&local, &t);
  char buf[3];
  // %02d ensures 24h format with leading zero (00-23)
  snprintf(buf, sizeof(buf), "%02d", t.tm_hour); 
  return String(buf);
}

String NTPS_getMM() {
  time_t local = NTPS_getLocalEpoch();
  struct tm t;
  localtime_r(&local, &t);
  char buf[3];
  snprintf(buf, sizeof(buf), "%02d", t.tm_min);
  return String(buf);
}

int NTPS_getSeconds() {
  time_t local = NTPS_getLocalEpoch();
  struct tm t;
  localtime_r(&local, &t);
  return t.tm_sec;
}

String NTPS_getDate() {
  time_t local = NTPS_getLocalEpoch();
  struct tm t;
  localtime_r(&local, &t);
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d.%02d", t.tm_mday, t.tm_mon + 1); // tm_mon is 0-based
  return String(buf);
}
