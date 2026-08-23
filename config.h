#ifndef CONFIG_H
#define CONFIG_H

// Target board: Waveshare ESP32-C6-LCD-1.47 (ST7789, 172x320 IPS).
// Uncomment to use the Adafruit driver instead of the bundled custom one.
// The custom driver (ST7789_Custom.h) handles the 172x320 offsets and the
// remappable ESP32-C6 SPI pins below.
// #define USE_ADAFRUIT_ST7789

#define SCREEN_W  172
#define SCREEN_H  320

#define AP_NAME_PREFIX          "LcdClk_"         // Will be appended by device MAC
#define AP_PASS                 "pass1234"

// Once the station is connected, turn the AP off this long after the last AP
// client disconnects (saves power by dropping to station-only mode). The AP is
// brought back automatically if the station connection is later lost.
#define AP_AUTO_OFF_MS          (120000)

#define WIFI_PASS_EEPROM_ADDR   (0)
#define WIFI_PASS_SIZE          (32)
#define SSID_EEPROM_ADDR        (WIFI_PASS_EEPROM_ADDR + WIFI_PASS_SIZE)
#define SSID_SIZE               (32)
#define LOCATION_EEPROM_ADDR    (SSID_EEPROM_ADDR + SSID_SIZE)
#define EEPROM_SIZE             (WIFI_PASS_SIZE + SSID_SIZE + 1)

// ESP32-C6-LCD-1.47 onboard display pins (Waveshare wiki). Dedicated hardware
// SPI bus, used only by the weather screen.
#define TFT_MOSI  6
#define TFT_SCLK  7
#define TFT_CS    14
#define TFT_DC    15
#define TFT_RST   21
#define TFT_BL    22

// External 172x320 ST7789 digit panels (nixie-tube style, one HH:MM digit
// each). GPIO6/7 above aren't broken out on this board, so these panels get
// their own SCLK/MOSI, bit-banged in software (ST7789_Custom handles this via
// its useHwSpi flag) -- frame rate is a non-issue since digits update at most
// once a minute. RES/DC/BL/SCLK/MOSI are shared across all 4 panels; only CS
// is separate per panel.
#define NUM_DIGIT_SCREENS 4
#define TFT_CS_D0   0
#define TFT_CS_D1   1
#define TFT_CS_D2   2
#define TFT_CS_D3   3
#define TFT_EXT_SCLK 4
#define TFT_EXT_MOSI 5
#define TFT_EXT_RST  9
#define TFT_EXT_DC   12
#define TFT_EXT_BL   13

// Backlight PWM. Docs warn against full brightness for long periods, so default
// to ~50% duty (8-bit resolution, 128/255).
#define TFT_BL_FREQ       5000
#define TFT_BL_RES_BITS   8
#define TFT_BL_DUTY       128

// --- Weather (Open-Meteo) ---
#define WEATHER_API_URL            "https://api.open-meteo.com/v1/forecast"
#define WEATHER_FETCH_INTERVAL_MS  (60UL * 60UL * 1000UL)   // 1h, matches pls_reklama

#endif
