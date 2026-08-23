#ifndef LCDDISPLAY_H
#define LCDDISPLAY_H

#define C_BLACK 0x0000
#define C_WHITE 0xFFFF
#define C_RED 0xF800
#define C_GREEN 0x07E0
#define C_BLUE 0x001F
#define C_CYAN 0x07FF
#define C_MAGENTA 0xF81F
#define C_YELLOW 0xFFE0
#define C_ORANGE 0xFC00
#define C_GRAY 0xC618

enum FontStyle {
  Font9pt,
  Font12pt,
  Font18pt,
  Font24pt,
  FontWeather   // 12pt-equivalent, extended range incl. the degree sign (0xB0)
};

extern void LCD_init(void);
extern void LCD_clear(void);
extern void LCD_textSize(int txtSize);
extern void LCD_color(uint16_t c);
extern void LCD_write(String msg);
extern void LCD_setFont(FontStyle id);
extern void LCD_setX(int x);
extern int LCD_getX(void);
extern int LCD_getY(void);
extern void LCD_setCursor(int x, int y);
extern void LCD_writeCentered(String msg, int y);
extern void LCD_clearStringArea(String msg);
// Draws a weather icon for one of weather.cpp's categories ("clear","partly",
// "clouds","fog","drizzle","rain","snow","thunder"), centered horizontally at
// baseline y, sized by radius r.
extern void LCD_drawWeatherIcon(String category, int cy, int r);
// Same icon, plus numStr (in numFont) and unitStr (in FontWeather, for the
// degree sign) printed to its right on one row, icon flush near the left edge.
// Leaves FontWeather set afterward.
extern void LCD_drawWeatherRow(String category, int cy, int r, FontStyle numFont, String numStr, String unitStr);
extern uint16_t LCD_getBgColor(void);
extern void LCD_setBgColor(uint16_t color);
extern uint16_t LCD_getFgColor(void);
extern void LCD_setFgColor(uint16_t color);
extern void LCD_setBacklight(uint8_t percent);   // 0..100

// Color for the clock digits only (onboard HH:MM and/or the external digit
// panels) -- not the weather icon/temp/precip, which keep their own colors.
extern uint16_t LCD_getClockColor(void);    // loads from EEPROM (default C_YELLOW) on first call
extern void LCD_setClockColor(uint16_t color);   // persists to EEPROM

// 4 external bit-banged-SPI panels, one big digit each: hour tens, hour units,
// minute tens, minute units.
extern void DIGITS_init(void);
extern void DIGITS_show(char hTens, char hUnits, char mTens, char mUnits);
extern void DIGITS_setBacklight(uint8_t percent);   // 0..100

#endif
