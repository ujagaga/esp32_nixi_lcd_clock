#include <Arduino.h>
#include <math.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include "config.h"
#include "wifi_connection.h"
#include "weather.h"

struct Location { const char* name; double lat; double lon; };

// A short predefined list, editable here. Index 0 (Novi Sad) is the default
// on first boot / unset EEPROM.
static const Location LOCATIONS[] = {
  { "Novi Sad", 45.243, 19.819 },
  { "Belgrade", 44.787, 20.457 },
  { "Nis",      43.320, 21.896 },
  { "Subotica", 46.100, 19.667 },
};
static const int LOCATION_COUNT = sizeof(LOCATIONS) / sizeof(LOCATIONS[0]);

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

int WEATHER_getLocationCount(void){
  return LOCATION_COUNT;
}

String WEATHER_getLocationName(int index){
  if(index < 0 || index >= LOCATION_COUNT) return "";
  return String(LOCATIONS[index].name);
}

int WEATHER_getLocationIndex(void){
  if(locationIndex < 0){
    EEPROM.begin(EEPROM_SIZE);
    uint8_t stored = 0;
    EEPROM.get(LOCATION_EEPROM_ADDR, stored);
    locationIndex = (stored < LOCATION_COUNT) ? stored : 0;
  }
  return locationIndex;
}

void WEATHER_setLocationIndex(int index){
  if(index < 0 || index >= LOCATION_COUNT) return;
  if(index == WEATHER_getLocationIndex()) return;
  locationIndex = index;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(LOCATION_EEPROM_ADDR, (uint8_t)index);
  EEPROM.commit();
  hasData = false;      // force a re-fetch for the new location
  lastFetchMs = 0;
}

void WEATHER_init(void){
  WEATHER_getLocationIndex();   // load persisted choice (or default to Novi Sad) up front
}

static void fetchForecast(void){
  const Location& loc = LOCATIONS[WEATHER_getLocationIndex()];
  String url = String(WEATHER_API_URL) + "?latitude=" + String(loc.lat, 3) +
               "&longitude=" + String(loc.lon, 3) +
               "&current=temperature_2m,weather_code,is_day&timezone=auto&forecast_days=1";

  WiFiClientSecure client;
  client.setInsecure();   // open-meteo's cert isn't pinned; skip verification
  HTTPClient http;
  http.begin(client, url);
  int code = http.GET();
  if(code == 200){
    DynamicJsonDocument doc(1024);
    if(deserializeJson(doc, http.getStream()) == DeserializationError::Ok){
      JsonObject cur = doc["current"];
      currentTemp = (int)round((double)cur["temperature_2m"]);
      WeatherMeta meta = weatherMeta(cur["weather_code"].as<int>());
      currentDescription = meta.description;
      currentCategory = meta.category;
      hasData = true;
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
