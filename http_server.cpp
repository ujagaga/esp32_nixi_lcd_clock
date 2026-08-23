/*
 *  Author: Rada Berar
 *  email: ujagaga@gmail.com
 *
 *  HTTP server which generates the web browser pages. Its whole job is WiFi +
 *  weather-location setup: scan/pick a station AP + password, pick a weather
 *  location, save. Everything else runs standalone once connected.
 */

#include <WebServer.h>
#include "wifi_connection.h"
#include "config.h"
#include "esp32_nixi_oled_clock.h"
#include "weather.h"
#include "http_ui.h"

// --- Web server object ---
WebServer* webServer = nullptr;

// --- Page handlers ---
void showStartPage() {
  WIFIC_startScan();   // kick off the scan; page JS polls /aplist ~10s later

  String response = FPSTR(HTML_BEGIN);
  response += FPSTR(CONFIG_HTML_0);
  response += FPSTR(CONFIG_HTML_1);
  response += FPSTR(CONFIG_FORM_HEAD);

  response += "<input id='s' name='s' length=32 value='" + WIFIC_getStSSID() +
              "' placeholder='SSID (Leave blank for AP mode)'><br>";
  response += "<input id='p' name='p' length=32 placeholder='Password'><br>";
  response += "<label for='loc'>Weather location</label><br><select id='loc' name='loc'>";
  int selected = WEATHER_getLocationIndex();
  for(int i = 0; i < WEATHER_getLocationCount(); i++){
    response += "<option value='" + String(i) + "'";
    if(i == selected){
      response += " selected";
    }
    response += ">" + WEATHER_getLocationName(i) + "</option>";
  }
  response += "</select><br>";

  response += FPSTR(CONFIG_FORM_TAIL);
  response += FPSTR(HTML_END);
  webServer->send(200, "text/html", response);
}

static void showNotFound(void){
  webServer->send(404, "text/html; charset=iso-8859-1","<html><head> <title>404 Not Found</title></head><body><h1>Not Found</h1></body></html>");
}

static void showStatusPage(bool goToHome = false) {
  String response = FPSTR(HTML_BEGIN);
  response += "<h1>Connection Status</h1><p>";
  response += MAIN_getStatusMsg() + "</p>";
  if(goToHome){
    response += FPSTR(REDIRECT_HTML);
  }
  response += FPSTR(HTML_END);
  webServer->send(200, "text/html", response);
}

static void saveWiFi(void){
  String ssid = webServer->arg("s");
  String pass = webServer->arg("p");
  int loc = webServer->arg("loc").toInt();

  if((ssid.length() > 63) || (pass.length() > 63)){
      MAIN_setStatusMsg("Sorry, this module can only remember SSID and a PASSWORD up to 63 bytes long.");
      showStatusPage(true);
      return;
  }

  WEATHER_setLocationIndex(loc);

  String st_ssid = WIFIC_getStSSID();
  String st_pass = WIFIC_getStPass();

  if(st_ssid.equals(ssid) && st_pass.equals(pass)){
      MAIN_setStatusMsg("All parameters are already set as requested.");
      showStatusPage(true);
      return;
  }

  WIFIC_setStSSID(ssid);
  WIFIC_setStPass(pass);

  String http_statusMessage;

  if(ssid.length() > 3){
    http_statusMessage = "Saving settings and connecting to SSID: " + ssid;
  }else{
    http_statusMessage = "No SSID selected...";
  }

  MAIN_setStatusMsg(http_statusMessage);
  showStatusPage();

  WIFIC_stationMode();
}

static void apList(void){
  webServer->send(200, "text/plain", WIFIC_getApList());
}

// --- Public functions ---
void HTTP_SERVER_process(void){
  webServer->handleClient();
}

void HTTP_SERVER_init(void){
  if (webServer != nullptr) {
    delete webServer; // Clean up old one
  }
  webServer = new WebServer(80);

  webServer->on("/", HTTP_GET, showStartPage);
  webServer->on("/favicon.ico", HTTP_GET, showNotFound);
  webServer->on("/wifisave", HTTP_GET, saveWiFi);
  webServer->on("/aplist", HTTP_GET, apList);
  webServer->onNotFound(showStartPage);

  webServer->begin();
}
