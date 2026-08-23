#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>

extern void WEATHER_init(void);
extern void WEATHER_process(void);            // call from loop(); fetches only once connected

// Location list includes one trailing "Custom (GPS)" entry, at index
// WEATHER_getLocationCount()-1, whose coordinates come from WEATHER_setCustomLocation().
extern int WEATHER_getLocationCount(void);
extern String WEATHER_getLocationName(int index);
extern int WEATHER_getLocationIndex(void);     // loads from EEPROM on first call
extern void WEATHER_setLocationIndex(int index); // persists to EEPROM
extern float WEATHER_getCustomLat(void);
extern float WEATHER_getCustomLon(void);
extern void WEATHER_setCustomLocation(float lat, float lon); // persists + selects the custom entry

extern bool WEATHER_hasData(void);
extern int WEATHER_getTemp(void);
extern String WEATHER_getDescription(void);
extern String WEATHER_getCategory(void);

// Precipitation timing for the rest of today, derived from the hourly forecast.
enum PrecipState {
  PRECIP_NONE,       // no rain/snow/drizzle/thunder now or expected before end of day
  PRECIP_CONTINUES,  // already precipitating, expected to continue all day
  PRECIP_STARTS,     // not precipitating now, will start later (see WEATHER_getPrecipTime)
  PRECIP_ENDS        // precipitating now, will end later (see WEATHER_getPrecipTime)
};
extern PrecipState WEATHER_getPrecipState(void);
extern String WEATHER_getPrecipTime(void);   // "HH:MM" of the transition; "" for NONE/CONTINUES

#endif
