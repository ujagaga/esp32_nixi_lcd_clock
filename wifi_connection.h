#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

extern void WIFIC_init(void);
extern void WIFIC_process(void);
extern void WIFIC_stationMode(void);
extern void WIFIC_setStSSID(String new_ssid);
extern void WIFIC_setStPass(String new_pass);
extern void WIFIC_startScan(void);            // begin an async WiFi scan
extern String WIFIC_getApList(void);          // results of the last scan ("" if not ready)
extern String WIFIC_getStSSID(void);
extern String WIFIC_getStPass(void);
extern char* WIFIC_getDeviceName(void);       
extern String WIFIC_getStationIp(void);       
extern bool WIFIC_stationConnected(void);     
extern String WIFIC_getApIp(void);

#endif
