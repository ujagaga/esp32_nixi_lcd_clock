#ifndef TRANSLATIONS_H
#define TRANSLATIONS_H

#include <Arduino.h>
#include "weather.h"

// On-screen text for each weather.cpp category ("clear","partly","clouds","fog",
// "drizzle","rain","snow","thunder" -- see weatherMeta() in weather.cpp). Edit
// here to change wording or language.
static inline String translateWeatherCategory(String category){
  if(category == "clear")   return "Vedro";
  if(category == "partly")  return "Delimično";
  if(category == "clouds")  return "Oblačno";
  if(category == "fog")     return "Magla";
  if(category == "drizzle") return "Rosulja";
  if(category == "rain")    return "Kiša";
  if(category == "snow")    return "Sneg";
  if(category == "thunder") return "Grmljavina";
  return category;
}

// Compact rain-timing line -- kept short since the panel is only 172px wide.
// Returns "" for PRECIP_NONE (caller should skip drawing the line entirely).
static inline String translatePrecipMessage(PrecipState state, String time){
  if(state == PRECIP_STARTS)    return "Kiša " + time;
  if(state == PRECIP_ENDS)      return "Kraj " + time;
  if(state == PRECIP_CONTINUES) return "Kiša ceo dan";
  return "";
}

// The embedded FreeMonoBold12pt8b font has no UTF-8 support (Adafruit_GFX reads
// raw bytes, not decoded codepoints) -- it only has single-byte glyphs for
// ASCII, the degree sign (0xB0), and 5 custom slots (0xB1-0xB5) for the Serbian
// diacritics used above. This converts the UTF-8 text from translateWeatherCategory()
// into those single-byte codes before rendering. Call it right before LCD_write*.
static inline String remapSerbianDiacritics(const String& s){
  String out;
  out.reserve(s.length());
  for(size_t i = 0; i < s.length(); ){
    uint8_t b0 = (uint8_t)s[i];
    if(b0 == 0xC4 && i + 1 < s.length()){
      uint8_t b1 = (uint8_t)s[i + 1];
      if(b1 == 0x8D){ out += (char)0xB1; i += 2; continue; }   // c-caron (c)
      if(b1 == 0x87){ out += (char)0xB2; i += 2; continue; }   // c-acute (c)
      if(b1 == 0x91){ out += (char)0xB5; i += 2; continue; }   // dj (d)
    }
    if(b0 == 0xC5 && i + 1 < s.length()){
      uint8_t b1 = (uint8_t)s[i + 1];
      if(b1 == 0xA1){ out += (char)0xB3; i += 2; continue; }   // s-caron (s)
      if(b1 == 0xBE){ out += (char)0xB4; i += 2; continue; }   // z-caron (z)
    }
    out += (char)b0;
    i += 1;
  }
  return out;
}

#endif
