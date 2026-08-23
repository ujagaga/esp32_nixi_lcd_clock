#include "wifi_connection.h"
#include "config.h"
#include "http_server.h"
#include "lcd_display.h"
#include "NTPSync.h"
#include "weather.h"
#include "translations.h"
#include "esp32_nixi_lcd_clock.h"

// After connecting, show the status screen for this long before switching to
// a clock-only display.
#define STATUS_SCREEN_MS  (10000)

enum DisplayPhase {
  PHASE_HOTSPOT,   // not connected: show hotspot SSID/PASS/IP
  PHASE_STATUS,    // just connected: show station SSID/IP + time, briefly
  PHASE_CLOCK      // after STATUS_SCREEN_MS: just the time
};

static String statusMessage = "";       /* This is set and requested from other modules. */
static DisplayPhase phase = PHASE_HOTSPOT;
static uint32_t phaseChangedAt = 0;
static String lastClockHHMM = "";       /* last drawn "HH:MM" in PHASE_CLOCK, "" forces a redraw */

void MAIN_setStatusMsg(String msg){
  statusMessage = msg;
}

String MAIN_getStatusMsg(void){
  return statusMessage;
}

static void display_hotspot_info()
{
  LCD_clear();
  LCD_color(C_YELLOW);
  LCD_write("\nWiFi SSID:\n");
  LCD_color(C_WHITE);
  LCD_write(String(WIFIC_getDeviceName()));
  LCD_color(C_YELLOW);
  LCD_write("\nPASS:\n");
  LCD_color(C_WHITE);
  LCD_write(AP_PASS);
  LCD_color(C_YELLOW);
  LCD_write("\nIP:\n");
  LCD_color(C_WHITE);
  LCD_write(WIFIC_getApIp());
}

static void display_station_info()
{
  LCD_clear();
  LCD_color(C_YELLOW);
  LCD_write("\nConnected to:\n");
  LCD_color(C_WHITE);
  LCD_write(WIFIC_getStSSID());
  LCD_color(C_YELLOW);
  LCD_write("\nIP:\n");
  LCD_color(C_WHITE);
  LCD_write(WIFIC_getStationIp());
  LCD_color(C_YELLOW);
  LCD_write("\n\nTime:\n");
  LCD_color(C_WHITE);
  LCD_write(NTPS_hasSynced() ? (NTPS_getHH() + ":" + NTPS_getMM()) : "syncing...");
}

// Onboard screen: weather only. The HH:MM digits themselves are shown on the
// 4 external panels (see DIGITS_show), not here.
static void display_weather()
{
  LCD_clear();

  if(WEATHER_hasData()){
    LCD_setFont(FontWeather);
    LCD_textSize(1);
    LCD_color(C_WHITE);
    LCD_writeCentered(String(WEATHER_getTemp()) + "\xB0" + "C", 240);
    LCD_writeCentered(remapSerbianDiacritics(translateWeatherCategory(WEATHER_getCategory())), 264);

    PrecipState precip = WEATHER_getPrecipState();
    if(precip != PRECIP_NONE){
      LCD_color(C_CYAN);
      LCD_writeCentered(remapSerbianDiacritics(translatePrecipMessage(precip, WEATHER_getPrecipTime())), 288);
    }
  }
}

// Poll the BOOT button (active low). Before the station connects, a debounced
// press connects immediately instead of waiting out STA_CONNECT_DELAY_MS;
// once connected, it instead cycles the backlight between 10% and 50%.
static void checkButton(void)
{
  static bool wasPressed = false;
  static uint8_t backlightPercent = 50;   // matches the LCD_init() default duty
  bool pressed = (digitalRead(BUTTON_PIN) == LOW);

  if(pressed && !wasPressed){
    delay(20);                                 // simple debounce
    if(digitalRead(BUTTON_PIN) == LOW){
      if(WIFIC_stationConnected()){
        backlightPercent = (backlightPercent == 10) ? 50 : 10;
        LCD_setBacklight(backlightPercent);
        DIGITS_setBacklight(backlightPercent);
      }else{
        WIFIC_stationMode();
      }
      wasPressed = true;
    }
  }else if(!pressed){
    wasPressed = false;
  }
}

void setup(void)
{
  /* Need to wait for background processes to complete. Otherwise trouble with gpio.*/
  delay(100);
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  WIFIC_init();
  HTTP_SERVER_init();
  LCD_init();
  DIGITS_init();
  NTPS_init();
  WEATHER_init();
  display_hotspot_info();
}

void loop(void){
  HTTP_SERVER_process();
  WIFIC_process();
  NTPS_process();
  WEATHER_process();
  checkButton();

  bool connected = WIFIC_stationConnected();

  if(connected && phase == PHASE_HOTSPOT){
    phase = PHASE_STATUS;
    phaseChangedAt = millis();
    display_station_info();
  }else if(!connected && phase != PHASE_HOTSPOT){
    phase = PHASE_HOTSPOT;
    display_hotspot_info();
  }else if(phase == PHASE_STATUS && (millis() - phaseChangedAt) > STATUS_SCREEN_MS){
    phase = PHASE_CLOCK;
    lastClockHHMM = "";   // force the first clock draw
  }

  if(phase == PHASE_CLOCK && NTPS_hasSynced()){
    String hh = NTPS_getHH();
    String mm = NTPS_getMM();
    String hhmm = hh + ":" + mm;
    if(hhmm != lastClockHHMM){
      lastClockHHMM = hhmm;
      display_weather();
      DIGITS_show(hh[0], hh[1], mm[0], mm[1]);
    }
  }
}
