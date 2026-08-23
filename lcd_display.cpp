#include "config.h"

#ifdef USE_ADAFRUIT_ST7789
#error "USE_ADAFRUIT_ST7789 does not support the 5-screen setup (external panels are bit-banged, which Adafruit_ST7789 here isn't wired for) -- leave USE_ADAFRUIT_ST7789 undefined to use the custom driver."
#endif

#include "ST7789_Custom.h"

static ST7789_Custom panelWeather(TFT_CS, TFT_RST, TFT_DC, TFT_BL, TFT_SCLK, TFT_MOSI, true);
static ST7789_Custom panelDigit[NUM_DIGIT_SCREENS] = {
  ST7789_Custom(TFT_CS_D0, TFT_EXT_RST, TFT_EXT_DC, TFT_EXT_BL, TFT_EXT_SCLK, TFT_EXT_MOSI, false),
  ST7789_Custom(TFT_CS_D1, TFT_EXT_RST, TFT_EXT_DC, TFT_EXT_BL, TFT_EXT_SCLK, TFT_EXT_MOSI, false),
  ST7789_Custom(TFT_CS_D2, TFT_EXT_RST, TFT_EXT_DC, TFT_EXT_BL, TFT_EXT_SCLK, TFT_EXT_MOSI, false),
  ST7789_Custom(TFT_CS_D3, TFT_EXT_RST, TFT_EXT_DC, TFT_EXT_BL, TFT_EXT_SCLK, TFT_EXT_MOSI, false),
};
static ST7789_Custom* panels[1 + NUM_DIGIT_SCREENS] = {
  &panelWeather, &panelDigit[0], &panelDigit[1], &panelDigit[2], &panelDigit[3]
};
static ST7789_Custom* activePanel = &panelWeather;

#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

#include "lcd_display.h"

static uint16_t bgColor = C_BLACK;
static uint16_t fgColor = C_YELLOW;

void LCD_init()
{
  for(int i = 0; i < 1 + NUM_DIGIT_SCREENS; i++){
    panels[i]->begin();
  }
  // Weather screen: wide landscape layout for temp/description text.
  panelWeather.setRotation(1);
  // Digit screens: tall portrait, one big glyph each.
  for(int i = 0; i < NUM_DIGIT_SCREENS; i++){
    panelDigit[i].setRotation(0);
  }

  for(int i = 0; i < 1 + NUM_DIGIT_SCREENS; i++){
    panels[i]->fillScreen(bgColor);
    panels[i]->setCursor(0, 0);
    panels[i]->setTextSize(1);
    panels[i]->setTextColor(fgColor);
  }
}

void LCD_select(LcdScreen screen){
  activePanel = panels[screen];
}

void LCD_setFont(FontStyle id){
  if(id == Font24pt){
    activePanel->setFont(&FreeMonoBold24pt7b);
  }else if(id == Font18pt){
    activePanel->setFont(&FreeMonoBold18pt7b);
  }else if(id == Font12pt){
    activePanel->setFont(&FreeMonoBold12pt7b);
  }else{
    activePanel->setFont(&FreeMonoBold9pt7b);
  }
}

void LCD_clear()
{
  activePanel->fillScreen(bgColor);
  activePanel->setCursor(0, 0);
  activePanel->setTextColor(fgColor);
  activePanel->setTextSize(1);
  LCD_setFont(Font9pt);
}

void LCD_setX(int x){
  int y = activePanel->getCursorY();
  activePanel->setCursor(x, y);
}

int LCD_getX(void){
  return activePanel->getCursorX();
}

int LCD_getY(void){
  return activePanel->getCursorY();
}

void LCD_setCursor(int x, int y){
  activePanel->setCursor(x, y);
}

// Write msg horizontally centered on the current screen at baseline y.
void LCD_writeCentered(String msg, int y){
  int16_t x1, y1;
  uint16_t w, h;
  activePanel->getTextBounds(msg, 0, y, &x1, &y1, &w, &h);
  int x = (activePanel->width() - (int)w) / 2;
  activePanel->setCursor(x, y);
  activePanel->print(msg);
}

// Clear the active screen and draw one big digit, centered in both axes
// (nixie-tube look for the external digit panels).
void LCD_writeBigDigit(char digit){
  LCD_clear();
  LCD_setFont(Font24pt);
  activePanel->setTextSize(4);   // tune once flashed -- can't verify pixel fit from here

  char buf[2] = { digit, 0 };
  int16_t x1, y1;
  uint16_t w, h;
  activePanel->getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
  int x = (activePanel->width() - (int)w) / 2 - x1;
  int y = (activePanel->height() - (int)h) / 2 - y1;
  activePanel->setCursor(x, y);
  activePanel->print(buf);
}

void LCD_textSize(int txtSize)
{
  activePanel->setTextSize(txtSize);
}

void LCD_color(uint16_t c)
{
  activePanel->setTextColor(c);
}

uint16_t LCD_getBgColor(void){
  return bgColor;
}

void LCD_setBgColor(uint16_t color){
  bgColor = color;
}

uint16_t LCD_getFgColor(void){
  return fgColor;
}

void LCD_setFgColor(uint16_t color){
  fgColor = color;
}

// NOTE: FreeType fonts draw on top of base line, so at coordinates 0,0 the text starts outside the screen.
// First set font size and write a new line character. This will set the cursor at the right place for first row.
void LCD_write(String msg)
{
  activePanel->print(msg);
}

void LCD_clearStringArea(String msg) {
  int16_t x1, y1;
  uint16_t w, h;

  // This calculates the bounding box of the string
  activePanel->getTextBounds(msg, activePanel->getCursorX(), activePanel->getCursorY(), &x1, &y1, &w, &h);

  // Fill that box with black
  activePanel->fillRect(x1, y1, w, h, C_BLACK);
}
