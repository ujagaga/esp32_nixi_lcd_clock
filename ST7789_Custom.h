#ifndef ST7789_CUSTOM_H
#define ST7789_CUSTOM_H

#include <Adafruit_GFX.h>
#include <SPI.h>
#include "config.h"

// 172x320 panel is centered on the ST7789 240x320 GRAM -> 34px column offset.
#define X_OFFSET_DEFAULT 34
#define Y_OFFSET_DEFAULT 0

class ST7789_Custom : public Adafruit_GFX {
  public:
    // useHwSpi: true drives sclk/mosi via the ESP32 hardware SPI peripheral
    // (only one panel on the board can do this -- the onboard screen, whose
    // sclk/mosi aren't shared with anything else). false bit-bangs sclk/mosi
    // as plain GPIO, for panels wired to pins that aren't on the hardware SPI
    // bus; fine since these panels redraw at most once a minute.
    ST7789_Custom(uint8_t cs, uint8_t rst, uint8_t dc, uint8_t bl, uint8_t sclk, uint8_t mosi, bool useHwSpi)
      : Adafruit_GFX(SCREEN_W, SCREEN_H),
        _cs(cs), _rst(rst), _dc(dc), _bl(bl), _sclk(sclk), _mosi(mosi), _hw(useHwSpi) {
        _x_offset = X_OFFSET_DEFAULT;
        _y_offset = Y_OFFSET_DEFAULT;
    }

    void begin() {
      pinMode(_dc, OUTPUT);
      pinMode(_cs, OUTPUT);
      digitalWrite(_cs, HIGH);   // deselected until this panel's turn on the shared bus
      pinMode(_rst, OUTPUT);

      // Backlight via PWM (Arduino-ESP32 v3.x ledc API). Several panels can
      // share one BL pin, so only attach each physical pin once.
      if (!(blAttachedMask & (1UL << _bl))) {
        ledcAttach(_bl, TFT_BL_FREQ, TFT_BL_RES_BITS);
        ledcWrite(_bl, TFT_BL_DUTY);
        blAttachedMask |= (1UL << _bl);
      }

      if (_hw) {
        if (!hwSpiStarted) {
          SPI.begin(_sclk, -1, _mosi, -1);
          SPI.setFrequency(24000000);
          SPI.setDataMode(SPI_MODE3);
          hwSpiStarted = true;
        }
      } else {
        pinMode(_sclk, OUTPUT);
        pinMode(_mosi, OUTPUT);
        digitalWrite(_sclk, LOW);
      }

      digitalWrite(_rst, HIGH); delay(50);
      digitalWrite(_rst, LOW);  delay(50);
      digitalWrite(_rst, HIGH); delay(150);

      busAcquire();
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
      busAcquire();
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
      busAcquire();
      setAddrWindow(x, y, 1, 1);
      write16(color);
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
      if ((x >= _width) || (y >= _height)) return;
      busAcquire();
      setAddrWindow(x, y, w, h);
      for (uint32_t i = 0; i < (uint32_t)w * h; i++) write16(color);
    }

    // --- Shared SPI bus arbitration ---
    // Several panels can share one physical bus (their CS lines differ, everything
    // else is common). Only one CS may be low at a time, so acquiring a different
    // panel's bus deasserts whichever panel was previously selected.
    void busRelease() {
      digitalWrite(_cs, HIGH);
      if (activeInstance == this) activeInstance = nullptr;
    }
    void busAcquire() {
      if (activeInstance != this) {
        if (activeInstance != nullptr) digitalWrite(activeInstance->_cs, HIGH);
        digitalWrite(_cs, LOW);
        activeInstance = this;
      }
      digitalWrite(_dc, HIGH);
    }

  private:
    uint8_t _cs, _rst, _dc, _bl, _sclk, _mosi;
    bool _hw;
    uint16_t _x_offset, _y_offset;

    static inline ST7789_Custom* activeInstance = nullptr;
    static inline bool hwSpiStarted = false;
    static inline uint32_t blAttachedMask = 0;

    void writeByte(uint8_t b) {
      if (_hw) { SPI.write(b); return; }
      // Bit-bang MSB-first, mode 0 (idle low, sample on rising edge) -- matches
      // how the panel actually behaves on this SoC even over hardware SPI.
      for (int8_t i = 7; i >= 0; i--) {
        digitalWrite(_mosi, (b >> i) & 1 ? HIGH : LOW);
        digitalWrite(_sclk, HIGH);
        digitalWrite(_sclk, LOW);
      }
    }
    void write16(uint16_t v) {
      if (_hw) { SPI.write16(v); return; }
      writeByte(v >> 8); writeByte(v & 0xFF);
    }

    void sendCmd(uint8_t c) { digitalWrite(_dc, LOW); writeByte(c); }
    void sendData(uint8_t d) { digitalWrite(_dc, HIGH); writeByte(d); }

    void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
      uint16_t x0 = x + _x_offset, x1 = x + w - 1 + _x_offset;
      uint16_t y0 = y + _y_offset, y1 = y + h - 1 + _y_offset;
      sendCmd(0x2A); sendData(x0 >> 8); sendData(x0 & 0xFF); sendData(x1 >> 8); sendData(x1 & 0xFF);
      sendCmd(0x2B); sendData(y0 >> 8); sendData(y0 & 0xFF); sendData(y1 >> 8); sendData(y1 & 0xFF);
      sendCmd(0x2C); digitalWrite(_dc, HIGH);
    }
};

#endif
