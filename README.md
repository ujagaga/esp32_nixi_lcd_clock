# ESP32 Nixie OLED Clock

A WiFi clock built from 5 ST7789 TFT screens: 4 external 172x320 panels each show one
HH:MM digit (nixie-tube look), and the onboard 172x320 screen of the driving
ESP32-C6-LCD-1.47 board shows a live weather forecast.

## Hardware

- **Driver board**: Waveshare ESP32-C6-LCD-1.47 (ST7789, 172x320 IPS onboard).
- **4 external panels**: 1.47" ST7789, 172x320, pinout SCL/SDA/RES/DC/CS/BL.

### Pinout

| Signal | Onboard weather screen | External digit panels |
|---|---|---|
| SCLK | 7 (hardware SPI) | 18 (bit-banged, shared by all 4) |
| MOSI | 6 (hardware SPI) | 19 (bit-banged, shared by all 4) |
| RES  | 21 | 20 (shared by all 4) |
| DC   | 15 | 23 (shared by all 4) |
| BL   | 22 | 17 (shared by all 4) |
| CS   | 14 | 2 / 3 / 10 / 11 (one per panel) |

Digit panels 0-3 = hour tens, hour units, minute tens, minute units.

GPIO12/GPIO13 are the ESP32-C6's fixed native-USB D-/D+ pins (not remappable) — never
assign them to anything else, or the board loses its `/dev/ttyACM*` serial/flash port as
soon as your firmware configures them as GPIO. GPIO9 is the onboard BOOT button, also
fixed. GPIO8 is left free in case it's wired to an onboard status LED.

The onboard screen's SCLK/MOSI (GPIO6/7) aren't broken out on this board, so the external
panels can't share that bus — they get their own SCLK/MOSI, bit-banged in software
(`ST7789_BitBang` in `lcd_display.cpp`). Digits update at most once a minute, so bit-bang
speed is a non-issue. The external panel pins above are a proposal (the panels aren't
wired yet) — verify against the board's actual header before soldering, then update
`config.h` if they differ.

## Building / flashing

```
tools/setup_env.sh     # one-time: installs arduino-cli, ESP32 core, required libraries
tools/build.sh          # compile
tools/build.sh upload   # compile + flash (uses PORT from .env, default /dev/ttyACM0)
```

Set `Tools -> Partition Scheme -> Huge APP (3MB No OTA/1MB SPIFFS)` if building from the
Arduino IDE instead.

## First boot / setup

1. Device starts a WiFi hotspot (`LcdClk_<MAC>`, password in `config.h` as `AP_PASS`).
2. Connect to it and open the device's AP IP in a browser (shown on the weather screen).
3. Pick a station WiFi network (scanned live), enter its password, pick a weather location
   from the dropdown, and save.
4. Once connected to that network, timekeeping (NTP) and weather fetching start
   automatically. The hotspot drops after `AP_AUTO_OFF_MS` of no AP clients to save power,
   and comes back if the station connection is ever lost.

The same page (device's IP, port 80) can be revisited any time to change the WiFi network
or weather location.

## Weather

Fetched from [Open-Meteo](https://api.open-meteo.com/v1/forecast) (no API key needed),
throttled to once per hour. Predefined locations live in `weather.cpp` (`LOCATIONS[]`) —
edit that list to add/remove cities; default is Novi Sad.

## License

MIT, see `LICENSE.txt`.
