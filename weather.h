#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>

extern void WEATHER_init(void);
extern void WEATHER_process(void);            // call from loop(); fetches only once connected

extern int WEATHER_getLocationCount(void);
extern String WEATHER_getLocationName(int index);
extern int WEATHER_getLocationIndex(void);     // loads from EEPROM on first call
extern void WEATHER_setLocationIndex(int index); // persists to EEPROM

extern bool WEATHER_hasData(void);
extern int WEATHER_getTemp(void);
extern String WEATHER_getDescription(void);
extern String WEATHER_getCategory(void);

#endif
