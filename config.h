#ifndef CONFIG_H
#define CONFIG_H

// Target board: Waveshare ESP32-C6-LCD-1.47 (ST7789, 172x320 IPS).
// Uncomment to use the Adafruit driver instead of the bundled custom one.
// The custom driver (in lcd_display.cpp) handles the 172x320 offsets and the
// remappable ESP32-C6 SPI pins below.
// #define USE_ADAFRUIT_ST7789

// Comment out for a single-screen setup: no external panels wired, the onboard
// screen shows a big clock with a small weather icon+temp underneath. Leave
// defined to drive the 4 external bit-banged digit panels for HH:MM, with the
// onboard screen showing the larger weather forecast (icon+temp+precip) only.
//#define USE_EXTERNAL_DIGIT_PANELS

#define SCREEN_W  172
#define SCREEN_H  320

#define AP_NAME_PREFIX          "LcdClk_"         // Will be appended by device MAC
#define AP_PASS                 "pass1234"

// The AP stays up at least this long after boot regardless of clients (grace
// period to reconfigure); after that, it's only torn down once zero clients
// are connected to it (never kicks an active client). Brought back automatically
// if the station connection is later lost.
#define AP_AUTO_OFF_MS          (120000)

// Don't start trying to connect as a station until this long after boot. The
// ESP32 has a single radio shared between AP and STA, so a STA connect/retry
// cycle running concurrently starves WiFi scans (used by the config page) of
// airtime -- delaying it gives the setup/scan window the radio to itself.
#define STA_CONNECT_DELAY_MS    (60000)

#define WIFI_PASS_EEPROM_ADDR    (0)
#define WIFI_PASS_SIZE           (32)
#define SSID_EEPROM_ADDR         (WIFI_PASS_EEPROM_ADDR + WIFI_PASS_SIZE)
#define SSID_SIZE                (32)
#define LOCATION_EEPROM_ADDR     (SSID_EEPROM_ADDR + SSID_SIZE)
#define CUSTOM_LAT_EEPROM_ADDR   (LOCATION_EEPROM_ADDR + 1)
#define CUSTOM_LON_EEPROM_ADDR   (CUSTOM_LAT_EEPROM_ADDR + sizeof(float))
#define CLOCK_COLOR_EEPROM_ADDR  (CUSTOM_LON_EEPROM_ADDR + sizeof(float))
#define EEPROM_SIZE              (CLOCK_COLOR_EEPROM_ADDR + sizeof(uint16_t))

// ESP32-C6-LCD-1.47 onboard display pins (Waveshare wiki).
#define TFT_MOSI  6
#define TFT_SCLK  7
#define TFT_CS    14
#define TFT_DC    15
#define TFT_RST   21
#define TFT_BL    22

// Backlight PWM. Docs warn against full brightness for long periods, so default
// to ~50% duty (8-bit resolution, 128/255).
#define TFT_BL_FREQ       5000
#define TFT_BL_RES_BITS   8
#define TFT_BL_DUTY       128

// Onboard BOOT pushbutton (active low, has external pull-up).
#define BUTTON_PIN        9

// External digit panels (4x ST7789 172x320, bit-banged SPI -- onboard SCLK/MOSI
// aren't broken out on this board). SCLK/MOSI/RST/DC/BL are wired to all 4 panels
// in parallel; only CS is per-panel. Not yet wired -- pins below are a proposal,
// chosen to avoid BUTTON_PIN, the native-USB pins (12/13), the onboard TFT pins,
// and GPIO8 (likely the onboard RGB status LED). Verify against the board's
// actual header before soldering.
#define DIGIT_SCLK  18
#define DIGIT_MOSI  19
#define DIGIT_RST   20
#define DIGIT_DC    23
#define DIGIT_BL    17
#define DIGIT_CS0   2   // hour tens
#define DIGIT_CS1   3   // hour units
#define DIGIT_CS2   10  // minute tens
#define DIGIT_CS3   11  // minute units

// --- Weather (Open-Meteo) ---
#define WEATHER_API_URL            "https://api.open-meteo.com/v1/forecast"
#define WEATHER_FETCH_INTERVAL_MS  (60UL * 60UL * 1000UL)   // 1h, matches pls_reklama

#endif
