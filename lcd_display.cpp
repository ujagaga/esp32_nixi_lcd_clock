#include "config.h"
#include <EEPROM.h>

#ifdef USE_ADAFRUIT_ST7789
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
#else
#include <Adafruit_GFX.h>
#include <SPI.h>

// 172x320 panel is centered on the ST7789 240x320 GRAM -> 34px column offset.
#define X_OFFSET_DEFAULT 34
#define Y_OFFSET_DEFAULT 0

class ST7789_Custom : public Adafruit_GFX {
  public:
    ST7789_Custom() : Adafruit_GFX(SCREEN_W, SCREEN_H) {
        _x_offset = X_OFFSET_DEFAULT;
        _y_offset = Y_OFFSET_DEFAULT;
    }

    void begin() {
      pinMode(TFT_DC, OUTPUT);
      pinMode(TFT_RST, OUTPUT);
      pinMode(TFT_CS, OUTPUT);
      digitalWrite(TFT_CS, LOW);   // single SPI device, keep selected

      ledcAttach(TFT_BL, TFT_BL_FREQ, TFT_BL_RES_BITS);
      ledcWrite(TFT_BL, TFT_BL_DUTY);

      SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
      SPI.setFrequency(24000000);
      SPI.setDataMode(SPI_MODE3);

      digitalWrite(TFT_RST, HIGH); delay(50);
      digitalWrite(TFT_RST, LOW);  delay(50);
      digitalWrite(TFT_RST, HIGH); delay(150);

      sendCmd(0x01); delay(150); // Software Reset
      sendCmd(0x11); delay(150); // Exit Sleep
      sendCmd(0x3A); sendData(0x05); // 16-bit color
      sendCmd(0x21);                 // Inversion ON (required for this IPS panel)
      setRotation(0);                // Set initial orientation
      sendCmd(0x13); // Normal mode
      sendCmd(0x29); // Display on
    }

    void setRotation(uint8_t m) override {
      Adafruit_GFX::setRotation(m);
      sendCmd(0x36); // MADCTL
      switch (rotation) {
        case 0: // Portrait
          sendData(0x00);
          _width  = SCREEN_W;
          _height = SCREEN_H;
          _x_offset = X_OFFSET_DEFAULT;
          _y_offset = Y_OFFSET_DEFAULT;
          break;
        case 1: // Landscape (90 deg)
          sendData(0x60);
          _width  = SCREEN_H;
          _height = SCREEN_W;
          _x_offset = Y_OFFSET_DEFAULT;
          _y_offset = X_OFFSET_DEFAULT;
          break;
        case 2: // Portrait Inverted
          sendData(0xC0);
          _width  = SCREEN_W;
          _height = SCREEN_H;
          _x_offset = X_OFFSET_DEFAULT;
          _y_offset = Y_OFFSET_DEFAULT;
          break;
        case 3: // Landscape Inverted
          sendData(0xA0);
          _width  = SCREEN_H;
          _height = SCREEN_W;
          _x_offset = Y_OFFSET_DEFAULT;
          _y_offset = X_OFFSET_DEFAULT;
          break;
      }
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
      if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) return;
      setAddrWindow(x, y, 1, 1);
      SPI.write16(color);
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
      if ((x >= _width) || (y >= _height)) return;
      setAddrWindow(x, y, w, h);
      for (uint32_t i = 0; i < (uint32_t)w * h; i++) SPI.write16(color);
    }

  private:
    uint16_t _x_offset, _y_offset;

    void sendCmd(uint8_t c) { digitalWrite(TFT_DC, LOW); SPI.write(c); }
    void sendData(uint8_t d) { digitalWrite(TFT_DC, HIGH); SPI.write(d); }

    void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
      uint16_t x0 = x + _x_offset, x1 = x + w - 1 + _x_offset;
      uint16_t y0 = y + _y_offset, y1 = y + h - 1 + _y_offset;
      sendCmd(0x2A); sendData(x0 >> 8); sendData(x0 & 0xFF); sendData(x1 >> 8); sendData(x1 & 0xFF);
      sendCmd(0x2B); sendData(y0 >> 8); sendData(y0 & 0xFF); sendData(y1 >> 8); sendData(y1 & 0xFF);
      sendCmd(0x2C); digitalWrite(TFT_DC, HIGH);
    }
};

ST7789_Custom tft;
#endif

// Same 172x320 panel as onboard, driven over bit-banged SPI (SCLK/MOSI/DC shared
// across all 4, CS per-panel) since the onboard hardware SPI pins aren't broken
// out on this board.
#define DIGIT_X_OFFSET 34   // mirrors the onboard driver's GRAM-centering offset

class ST7789_BitBang : public Adafruit_GFX {
  public:
    ST7789_BitBang(uint8_t sclk, uint8_t mosi, uint8_t cs, uint8_t dc)
      : Adafruit_GFX(SCREEN_W, SCREEN_H), _sclk(sclk), _mosi(mosi), _cs(cs), _dc(dc) {}

    void begin() {
      pinMode(_sclk, OUTPUT);
      pinMode(_mosi, OUTPUT);
      pinMode(_cs, OUTPUT);
      pinMode(_dc, OUTPUT);
      digitalWrite(_sclk, LOW);
      digitalWrite(_cs, HIGH);

      digitalWrite(_cs, LOW);
      sendCmd(0x01); delay(150); // Software Reset
      sendCmd(0x11); delay(150); // Exit Sleep
      sendCmd(0x3A); sendData(0x05); // 16-bit color
      sendCmd(0x21);                 // Inversion ON (required for this IPS panel)
      sendCmd(0x36); sendData(0x00); // MADCTL, portrait
      sendCmd(0x13); // Normal mode
      sendCmd(0x29); // Display on
      digitalWrite(_cs, HIGH);
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
      if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) return;
      digitalWrite(_cs, LOW);
      setAddrWindow(x, y, 1, 1);
      write16(color);
      digitalWrite(_cs, HIGH);
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
      if ((x >= _width) || (y >= _height)) return;
      digitalWrite(_cs, LOW);
      setAddrWindow(x, y, w, h);
      for (uint32_t i = 0; i < (uint32_t)w * h; i++) write16(color);
      digitalWrite(_cs, HIGH);
    }

  private:
    uint8_t _sclk, _mosi, _cs, _dc;

    void writeByte(uint8_t b) {
      for (int8_t i = 7; i >= 0; i--) {
        digitalWrite(_mosi, (b >> i) & 0x01);
        digitalWrite(_sclk, HIGH);
        digitalWrite(_sclk, LOW);
      }
    }
    void write16(uint16_t v) { writeByte(v >> 8); writeByte(v & 0xFF); }
    void sendCmd(uint8_t c) { digitalWrite(_dc, LOW); writeByte(c); }
    void sendData(uint8_t d) { digitalWrite(_dc, HIGH); writeByte(d); }

    void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
      uint16_t x0 = x + DIGIT_X_OFFSET, x1 = x + w - 1 + DIGIT_X_OFFSET;
      uint16_t y0 = y, y1 = y + h - 1;
      sendCmd(0x2A); sendData(x0 >> 8); sendData(x0 & 0xFF); sendData(x1 >> 8); sendData(x1 & 0xFF);
      sendCmd(0x2B); sendData(y0 >> 8); sendData(y0 & 0xFF); sendData(y1 >> 8); sendData(y1 & 0xFF);
      sendCmd(0x2C); digitalWrite(_dc, HIGH);
    }
};

static ST7789_BitBang digitPanels[4] = {
  ST7789_BitBang(DIGIT_SCLK, DIGIT_MOSI, DIGIT_CS0, DIGIT_DC),
  ST7789_BitBang(DIGIT_SCLK, DIGIT_MOSI, DIGIT_CS1, DIGIT_DC),
  ST7789_BitBang(DIGIT_SCLK, DIGIT_MOSI, DIGIT_CS2, DIGIT_DC),
  ST7789_BitBang(DIGIT_SCLK, DIGIT_MOSI, DIGIT_CS3, DIGIT_DC),
};

#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include "FreeMonoBold12pt8b.h"
#include "FreeMonoBold175pt7b.h"

#include "lcd_display.h"

static uint16_t bgColor = C_BLACK;
static uint16_t fgColor = C_YELLOW;

#ifdef USE_ADAFRUIT_ST7789
void LCD_init()
{
  ledcAttach(TFT_BL, TFT_BL_FREQ, TFT_BL_RES_BITS);
  ledcWrite(TFT_BL, TFT_BL_DUTY);
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.init(SCREEN_W, SCREEN_H, SPI_MODE3);
  tft.setRotation(0);

  tft.fillScreen(bgColor);
  tft.setCursor(0, 0);
  tft.setTextSize(1);
  tft.setTextColor(fgColor);
}
#else
void LCD_init()
{
  tft.begin();
  // Set to landscape for a wide "bar" look. Use 0 or 2 for portrait.
  tft.setRotation(0);

  tft.fillScreen(bgColor);
  tft.setCursor(0, 0);
  tft.setTextSize(1);
  tft.setTextColor(fgColor);
}
#endif

void LCD_setFont(FontStyle id){
  if(id == Font24pt){
    tft.setFont(&FreeMonoBold24pt7b);
  }else if(id == Font18pt){
    tft.setFont(&FreeMonoBold18pt7b);
  }else if(id == Font12pt){
    tft.setFont(&FreeMonoBold12pt7b);
  }else if(id == FontWeather){
    tft.setFont(&FreeMonoBold12pt8b);
  }else{
    tft.setFont(&FreeMonoBold9pt7b);
  }
}

void LCD_clear()
{
  tft.fillScreen(bgColor);
  tft.setCursor(0, 0);
  tft.setTextColor(fgColor);
  tft.setTextSize(1);
  LCD_setFont(Font9pt);
}

void LCD_setX(int x){
  int y = tft.getCursorY();
  tft.setCursor(x, y);
}

int LCD_getX(void){
  return tft.getCursorX();
}

int LCD_getY(void){
  return tft.getCursorY();
}

void LCD_setCursor(int x, int y){
  tft.setCursor(x, y);
}

// Write msg horizontally centered on the current screen at baseline y.
void LCD_writeCentered(String msg, int y){
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(msg, 0, y, &x1, &y1, &w, &h);
  int x = (tft.width() - (int)w) / 2 - x1;   // x1: glyph's left-side bearing, must offset for it to truly center the ink
  tft.setCursor(x, y);
  tft.print(msg);
}

void LCD_textSize(int txtSize)
{
  tft.setTextSize(txtSize);
}

void LCD_color(uint16_t c)
{
  tft.setTextColor(c);
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

void LCD_setBacklight(uint8_t percent){
  if(percent > 100){
    percent = 100;
  }
  uint32_t maxDuty = (1u << TFT_BL_RES_BITS) - 1;
  ledcWrite(TFT_BL, (uint32_t)percent * maxDuty / 100);
}

static uint16_t clockColor = 0;   // 0: not yet loaded from EEPROM (also the "unset" sentinel value)

uint16_t LCD_getClockColor(void){
  if(clockColor == 0){
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(CLOCK_COLOR_EEPROM_ADDR, clockColor);
    if(clockColor == 0) clockColor = C_YELLOW;   // never set: default
  }
  return clockColor;
}

void LCD_setClockColor(uint16_t color){
  clockColor = color;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(CLOCK_COLOR_EEPROM_ADDR, color);
  EEPROM.commit();
}

// NOTE: FreeType fonts draw on top of base line, so at coordinates 0,0 the text starts outside the screen.
// First set font size and write a new line character. This will set the cursor at the right place for first row.
void LCD_write(String msg)
{
  tft.print(msg);
}

void LCD_clearStringArea(String msg) {
  int16_t x1, y1;
  uint16_t w, h;

  // This calculates the bounding box of the string
  tft.getTextBounds(msg, tft.getCursorX(), tft.getCursorY(), &x1, &y1, &w, &h);

  // Fill that box with black
  tft.fillRect(x1, y1, w, h, C_BLACK);
}

// 8 unit-ish direction vectors (x10), used to draw sun rays without pulling in <math.h>.
static const int8_t RAY_DX[8] = { 10, 7, 0, -7, -10, -7, 0, 7 };
static const int8_t RAY_DY[8] = { 0, 7, 10, 7, 0, -7, -10, -7 };

static void drawSun(int cx, int cy, int r, uint16_t color){
  tft.fillCircle(cx, cy, r / 2, color);
  for(int i = 0; i < 8; i++){
    int ix = cx + RAY_DX[i] * r * 6 / 100;
    int iy = cy + RAY_DY[i] * r * 6 / 100;
    int ox = cx + RAY_DX[i] * r / 10;
    int oy = cy + RAY_DY[i] * r / 10;
    tft.drawLine(ix, iy, ox, oy, color);
  }
}

static void drawCloud(int cx, int cy, int w, uint16_t color){
  tft.fillRoundRect(cx - w / 2, cy - w / 8, w, w / 3, w / 6, color);
  tft.fillCircle(cx - w / 4, cy - w / 6, w / 4, color);
  tft.fillCircle(cx + w / 6, cy - w / 4, w / 3, color);
}

// Short vertical dashes below a cloud, for drizzle/rain.
static void drawDrops(int cx, int cy, int r, int count, int len, uint16_t color){
  for(int i = 0; i < count; i++){
    int x = cx - r * 3 / 5 + i * (r * 6 / 5) / (count - 1 > 0 ? count - 1 : 1);
    tft.fillRect(x, cy, 2, len, color);
  }
}

static void drawWeatherIconShape(String category, int cx, int cy, int r){
  if(category == "clear"){
    drawSun(cx, cy, r, C_YELLOW);
  }else if(category == "partly"){
    drawSun(cx + r * 3 / 10, cy - r * 3 / 10, r * 6 / 10, C_YELLOW);
    drawCloud(cx - r / 10, cy + r * 2 / 10, r * 13 / 10, C_WHITE);
  }else if(category == "clouds"){
    drawCloud(cx, cy, r * 16 / 10, C_GRAY);
  }else if(category == "fog"){
    tft.fillRect(cx - r, cy - r / 2, r * 2, 3, C_GRAY);
    tft.fillRect(cx - r + r / 3, cy - r / 6, r * 2 - r / 3, 3, C_GRAY);
    tft.fillRect(cx - r, cy + r / 6, r * 2, 3, C_GRAY);
    tft.fillRect(cx - r + r / 4, cy + r / 2, r * 2 - r / 4, 3, C_GRAY);
  }else if(category == "drizzle"){
    drawCloud(cx, cy - r / 4, r * 13 / 10, C_GRAY);
    drawDrops(cx, cy + r / 3, r, 2, r / 4, C_CYAN);
  }else if(category == "rain"){
    drawCloud(cx, cy - r / 4, r * 13 / 10, C_GRAY);
    drawDrops(cx, cy + r / 3, r, 4, r * 2 / 5, C_CYAN);
  }else if(category == "snow"){
    drawCloud(cx, cy - r / 4, r * 13 / 10, C_GRAY);
    for(int i = 0; i < 3; i++){
      int x = cx - r * 3 / 5 + i * (r * 6 / 5) / 2;
      int y = cy + r / 3;
      tft.drawLine(x - 4, y, x + 4, y, C_WHITE);
      tft.drawLine(x, y - 4, x, y + 4, C_WHITE);
    }
  }else if(category == "thunder"){
    drawCloud(cx, cy - r / 4, r * 13 / 10, C_GRAY);
    int bx = cx, by = cy + r / 4;
    tft.fillTriangle(bx + r / 6, by, bx - r / 8, by + r / 3, bx + r / 12, by + r / 3, C_YELLOW);
    tft.fillTriangle(bx + r / 12, by + r / 3, bx - r / 4, by + r * 2 / 3, bx + r / 8, by + r / 3, C_YELLOW);
  }
}

void LCD_drawWeatherIcon(String category, int cy, int r){
  drawWeatherIconShape(category, tft.width() / 2, cy, r);
}

// Icon + temperature side by side on one row, icon flush near the left edge.
// numStr is drawn in numFont; unitStr (the degree sign + "C") is drawn right
// after it in FontWeather, since numFont may not have a degree-sign glyph.
// Caller sets color beforehand, same as LCD_writeCentered. Leaves FontWeather
// set on return.
void LCD_drawWeatherRow(String category, int cy, int r, FontStyle numFont, String numStr, String unitStr){
  int leftPad = 8;
  int iconD = r * 2;
  int gap = 10;
  int unitGap = 4;

  drawWeatherIconShape(category, leftPad + r, cy, r);

  int16_t x1, y1;
  uint16_t w, h;

  LCD_setFont(numFont);
  tft.getTextBounds(numStr, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(leftPad + iconD + gap - x1, cy - y1 - (int)h / 2);
  tft.print(numStr);
  int afterNumX = tft.getCursorX();

  LCD_setFont(FontWeather);
  tft.getTextBounds(unitStr, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(afterNumX + unitGap - x1, cy - y1 - (int)h / 2);
  tft.print(unitStr);
}

void DIGITS_init()
{
  pinMode(DIGIT_RST, OUTPUT);
  digitalWrite(DIGIT_RST, HIGH); delay(50);
  digitalWrite(DIGIT_RST, LOW);  delay(50);
  digitalWrite(DIGIT_RST, HIGH); delay(150);

  ledcAttach(DIGIT_BL, TFT_BL_FREQ, TFT_BL_RES_BITS);
  ledcWrite(DIGIT_BL, TFT_BL_DUTY);

  for (int i = 0; i < 4; i++) {
    digitPanels[i].begin();
    digitPanels[i].setFont(&FreeMonoBold175pt7b);
    digitPanels[i].setTextColor(LCD_getClockColor());
    digitPanels[i].fillScreen(C_BLACK);
  }
}

static void showDigit(ST7789_BitBang &panel, char digit)
{
  int16_t x1, y1;
  uint16_t w, h;
  String s = String(digit);
  panel.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_W - (int)w) / 2 - x1;
  int y = (SCREEN_H - (int)h) / 2 - y1;

  panel.fillScreen(C_BLACK);
  panel.setTextColor(LCD_getClockColor());
  panel.setCursor(x, y);
  panel.print(s);
}

void DIGITS_show(char hTens, char hUnits, char mTens, char mUnits)
{
  showDigit(digitPanels[0], hTens);
  showDigit(digitPanels[1], hUnits);
  showDigit(digitPanels[2], mTens);
  showDigit(digitPanels[3], mUnits);
}

void DIGITS_setBacklight(uint8_t percent)
{
  if(percent > 100){
    percent = 100;
  }
  uint32_t maxDuty = (1u << TFT_BL_RES_BITS) - 1;
  ledcWrite(DIGIT_BL, (uint32_t)percent * maxDuty / 100);
}
