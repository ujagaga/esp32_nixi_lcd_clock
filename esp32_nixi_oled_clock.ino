#include "wifi_connection.h"
#include "config.h"
#include "http_server.h"
#include "NTPSync.h"
#include "lcd_display.h"
#include "weather.h"
#include "esp32_nixi_oled_clock.h"

enum Operation {
  Init,
  WifiCredentials,
  ConnectToAp,
  ShowIp,
  ShowTime
};

static Operation state = Init;
static uint32_t stateChangedAt = 0;
static String statusMessage = "";         /* This is set and requested from other modules. */
static String timeDigits = "";            /* last drawn "HHMM", "" until the first draw */
static int lastDrawnTemp = 0;
static String lastDrawnDescription = "";

void MAIN_setStatusMsg(String msg){
  statusMessage = msg;
}

String MAIN_getStatusMsg(void){
  return statusMessage;
}

static void display_boudries()
{
  LCD_clear();
  LCD_color(C_YELLOW);
  LCD_write("...1\n....2\n.....3\n......4\n.......5");
}

static void display_wifi_credentials()
{
  LCD_select(SCREEN_WEATHER);
  LCD_clear();
  LCD_color(C_YELLOW);
  LCD_write("\nWiFi SSID:\n");
  LCD_color(C_WHITE);
  String message = String(WIFIC_getDeviceName());
  LCD_write(message);
  LCD_color(C_YELLOW);
  LCD_write("\nPASS:");
  LCD_color(C_WHITE);
  LCD_write(AP_PASS);
  LCD_color(C_YELLOW);
  LCD_write("\nIP:");
  LCD_color(C_WHITE);
  LCD_write(WIFIC_getApIp());
}


void setup(void)
{
  /* Need to wait for background processes to complete. Otherwise trouble with gpio.*/
  delay(100);
  Serial.begin(115200);
  WIFIC_init();
  HTTP_SERVER_init();
  LCD_init();
  NTPS_init();
  WEATHER_init();
}

void loop(void){
  HTTP_SERVER_process();
  WIFIC_process();
  // Timekeeping and weather only make sense once actually connected to the
  // chosen AP; both stay idle while only the setup hotspot is up.
  if(WIFIC_stationConnected()){
    NTPS_process();
    WEATHER_process();
  }

  // State machine
  switch(state){
    case Init:
    {
      // display_boudries();
      state = WifiCredentials;
      stateChangedAt = millis();
    }break;

    case WifiCredentials:
    {
      if((millis() - stateChangedAt) > 5){
        display_wifi_credentials();
        state = ConnectToAp;
        stateChangedAt = millis();
      }
    }break;

    case ConnectToAp:
    {
      if((millis() - stateChangedAt) > 5000){
        LCD_select(SCREEN_WEATHER);
        LCD_clear();
        LCD_write("\nWaiting for WiFi,\nNTP sync...");
        state = ShowIp;
        stateChangedAt = millis();
      }
    }break;

    case ShowIp:
    {
      String stationIp = WIFIC_getStationIp();
      if((stationIp.length() > 1)){
        LCD_select(SCREEN_WEATHER);
        LCD_color(C_YELLOW);
        LCD_write("\nConnected IP:\n");
        LCD_color(C_WHITE);
        LCD_write(stationIp);
        state = ShowTime;
        stateChangedAt = millis();
      }
    }break;

    default:
    {
      // Digit panels: redraw only the digit(s) that changed.
      if(NTPS_hasSynced()){
        String digits = NTPS_getHH() + NTPS_getMM();
        for(int i = 0; i < 4; i++){
          if(timeDigits.length() != 4 || digits[i] != timeDigits[i]){
            LCD_select((LcdScreen)(SCREEN_DIGIT0 + i));
            LCD_writeBigDigit(digits[i]);
          }
        }
        timeDigits = digits;
      }

      // Weather screen: redraw only when the fetched data actually changes.
      if(WEATHER_hasData() &&
         (WEATHER_getTemp() != lastDrawnTemp || WEATHER_getDescription() != lastDrawnDescription)){
        lastDrawnTemp = WEATHER_getTemp();
        lastDrawnDescription = WEATHER_getDescription();

        LCD_select(SCREEN_WEATHER);
        LCD_clear();
        LCD_color(C_YELLOW);
        LCD_setFont(Font18pt);
        LCD_write(WEATHER_getLocationName(WEATHER_getLocationIndex()) + "\n\n");
        LCD_setFont(Font24pt);
        LCD_textSize(2);
        LCD_color(C_WHITE);
        LCD_write(String(lastDrawnTemp) + "C\n");
        LCD_textSize(1);
        LCD_setFont(Font18pt);
        LCD_color(C_BLUE);
        LCD_write(lastDrawnDescription);
      }
    }break;
  }

}
