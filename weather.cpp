#include <Arduino.h>
#include <math.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include "config.h"
#include "wifi_connection.h"
#include "NTPSync.h"
#include "weather.h"

#define PRECIP_CATEGORY(cat) ((cat) == "drizzle" || (cat) == "rain" || (cat) == "snow" || (cat) == "thunder")
#define MAX_HOURLY 24

struct Location { const char* name; double lat; double lon; };

// A short predefined list, editable here. Index 0 (Novi Sad) is the default
// on first boot / unset EEPROM. WEATHER_getLocationCount() adds one more slot
// after these for "Custom (GPS)" (see WEATHER_setCustomLocation).
static const Location LOCATIONS[] = {
  { "Novi Sad",  45.243, 19.819 },
  { "Beograd",   44.787, 20.457 },
  { "Kikinda",   45.830, 20.460 },
  { "Zrenjanin", 45.378, 20.390 },
};
static const int LOCATION_COUNT = sizeof(LOCATIONS) / sizeof(LOCATIONS[0]);
#define CUSTOM_LOCATION_INDEX LOCATION_COUNT

struct WeatherMeta { const char* description; const char* category; };

// Open-Meteo weathercode -> {description, category}, mirroring pls_reklama's
// WEATHER_CODES table (English only here).
static WeatherMeta weatherMeta(int code){
  switch(code){
    case 0:  return {"Clear sky", "clear"};
    case 1:  return {"Mainly clear", "partly"};
    case 2:  return {"Partly cloudy", "partly"};
    case 3:  return {"Overcast", "clouds"};
    case 45: case 48: return {"Fog", "fog"};
    case 51: case 53: case 55: case 56: case 57: return {"Drizzle", "drizzle"};
    case 61: case 63: case 65: case 66: case 67: return {"Rain", "rain"};
    case 71: case 73: case 75: case 77: return {"Snow", "snow"};
    case 80: case 81: case 82: return {"Rain showers", "rain"};
    case 85: case 86: return {"Snow showers", "snow"};
    case 95: case 96: case 99: return {"Thunderstorm", "thunder"};
    default: return {"Unknown", "clouds"};
  }
}

static int locationIndex = -1;   // -1: not yet loaded from EEPROM
static unsigned long lastFetchMs = 0;
static bool hasData = false;
static int currentTemp = 0;
static String currentDescription = "";
static String currentCategory = "";

// Rest-of-today hourly forecast, cached at fetch time; WEATHER_getPrecipState()
// re-scans this against the current time on every call (cheap, <=24 entries).
static String hourlyTime[MAX_HOURLY];       // "HH:MM"
static String hourlyCategory[MAX_HOURLY];
static int hourlyCount = 0;

int WEATHER_getLocationCount(void){
  return LOCATION_COUNT + 1;   // + the trailing "Custom (GPS)" slot
}

String WEATHER_getLocationName(int index){
  if(index == CUSTOM_LOCATION_INDEX) return "Custom (GPS)";
  if(index < 0 || index >= LOCATION_COUNT) return "";
  return String(LOCATIONS[index].name);
}

int WEATHER_getLocationIndex(void){
  if(locationIndex < 0){
    EEPROM.begin(EEPROM_SIZE);
    uint8_t stored = 0;
    EEPROM.get(LOCATION_EEPROM_ADDR, stored);
    locationIndex = (stored <= CUSTOM_LOCATION_INDEX) ? stored : 0;
  }
  return locationIndex;
}

void WEATHER_setLocationIndex(int index){
  if(index < 0 || index > CUSTOM_LOCATION_INDEX) return;
  if(index == WEATHER_getLocationIndex()) return;
  locationIndex = index;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(LOCATION_EEPROM_ADDR, (uint8_t)index);
  EEPROM.commit();
  hasData = false;      // force a re-fetch for the new location
  lastFetchMs = 0;
}

static float customLat = NAN, customLon = NAN;   // NAN: not yet loaded from EEPROM

float WEATHER_getCustomLat(void){
  if(isnan(customLat)){
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(CUSTOM_LAT_EEPROM_ADDR, customLat);
    if(isnan(customLat)) customLat = LOCATIONS[0].lat;   // never set: fall back to Novi Sad
  }
  return customLat;
}

float WEATHER_getCustomLon(void){
  if(isnan(customLon)){
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(CUSTOM_LON_EEPROM_ADDR, customLon);
    if(isnan(customLon)) customLon = LOCATIONS[0].lon;
  }
  return customLon;
}

void WEATHER_setCustomLocation(float lat, float lon){
  customLat = lat;
  customLon = lon;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(CUSTOM_LAT_EEPROM_ADDR, lat);
  EEPROM.put(CUSTOM_LON_EEPROM_ADDR, lon);
  EEPROM.commit();
  WEATHER_setLocationIndex(CUSTOM_LOCATION_INDEX);
  hasData = false;      // force a re-fetch even if already on the custom slot
  lastFetchMs = 0;
}

void WEATHER_init(void){
  WEATHER_getLocationIndex();   // load persisted choice (or default to Novi Sad) up front
}

static void fetchForecast(void){
  int idx = WEATHER_getLocationIndex();
  double lat = (idx == CUSTOM_LOCATION_INDEX) ? WEATHER_getCustomLat() : LOCATIONS[idx].lat;
  double lon = (idx == CUSTOM_LOCATION_INDEX) ? WEATHER_getCustomLon() : LOCATIONS[idx].lon;
  String url = String(WEATHER_API_URL) + "?latitude=" + String(lat, 3) +
               "&longitude=" + String(lon, 3) +
               "&current=temperature_2m,weather_code,is_day&hourly=weather_code&timezone=auto&forecast_days=1";

  WiFiClientSecure client;
  client.setInsecure();   // open-meteo's cert isn't pinned; skip verification
  HTTPClient http;
  http.begin(client, url);
  int code = http.GET();
  if(code == 200){
    String payload = http.getString();
    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, payload);
    if(err == DeserializationError::Ok){
      JsonObject cur = doc["current"];
      currentTemp = (int)round((double)cur["temperature_2m"]);
      WeatherMeta meta = weatherMeta(cur["weather_code"].as<int>());
      currentDescription = meta.description;
      currentCategory = meta.category;
      hasData = true;

      JsonArray times = doc["hourly"]["time"];
      JsonArray codes = doc["hourly"]["weather_code"];
      hourlyCount = 0;
      for(size_t i = 0; i < times.size() && hourlyCount < MAX_HOURLY; i++){
        String t = times[i].as<String>();           // "2026-08-23T14:00"
        hourlyTime[hourlyCount] = t.substring(11, 16);            // "14:00"
        hourlyCategory[hourlyCount] = weatherMeta(codes[i].as<int>()).category;
        hourlyCount++;
      }
    }
  }
  http.end();
  lastFetchMs = millis();
}

void WEATHER_process(void){
  if(!WIFIC_stationConnected()) return;
  if(lastFetchMs != 0 && (millis() - lastFetchMs) < WEATHER_FETCH_INTERVAL_MS) return;
  fetchForecast();
}

bool WEATHER_hasData(void){ return hasData; }
int WEATHER_getTemp(void){ return currentTemp; }
String WEATHER_getDescription(void){ return currentDescription; }
String WEATHER_getCategory(void){ return currentCategory; }

// Finds the next hour (after now) where the precip/no-precip state differs
// from now, mirroring pls_reklama's _precip_message logic.
static int findPrecipTransition(bool precipNow){
  String nowHHMM = NTPS_getHH() + ":" + NTPS_getMM();
  for(int i = 0; i < hourlyCount; i++){
    if(hourlyTime[i] <= nowHHMM) continue;
    if(PRECIP_CATEGORY(hourlyCategory[i]) != precipNow) return i;
  }
  return -1;
}

PrecipState WEATHER_getPrecipState(void){
  if(!hasData || hourlyCount == 0) return PRECIP_NONE;
  bool precipNow = PRECIP_CATEGORY(currentCategory);
  int i = findPrecipTransition(precipNow);
  if(i >= 0) return precipNow ? PRECIP_ENDS : PRECIP_STARTS;
  return precipNow ? PRECIP_CONTINUES : PRECIP_NONE;
}

String WEATHER_getPrecipTime(void){
  if(!hasData || hourlyCount == 0) return "";
  bool precipNow = PRECIP_CATEGORY(currentCategory);
  int i = findPrecipTransition(precipNow);
  return (i >= 0) ? hourlyTime[i] : "";
}
