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

enum FontStyle {
  Font9pt,
  Font12pt,
  Font18pt,
  Font24pt
};

// One weather screen (onboard) + 4 digit screens (external panels, HH:MM).
enum LcdScreen {
  SCREEN_WEATHER,
  SCREEN_DIGIT0,
  SCREEN_DIGIT1,
  SCREEN_DIGIT2,
  SCREEN_DIGIT3
};

extern void LCD_init(void);
extern void LCD_select(LcdScreen screen);
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
extern void LCD_writeBigDigit(char digit);
extern void LCD_clearStringArea(String msg);
extern uint16_t LCD_getBgColor(void);
extern void LCD_setBgColor(uint16_t color);
extern uint16_t LCD_getFgColor(void);
extern void LCD_setFgColor(uint16_t color);

#endif
