/*
   SmartMirror

   A decimal clock on a 64x16 LED Matrix and a DHT22 temperature and humidity sensor.
   Applicable for ESP8266 boards (like NodeMCU). The current time is gathered via NTP.

   A project by Ben-Noah Engelhaupt (code@bengelhaupt.com) Github: bengelhaupt
   Published under GNU General Public License v3.0

   http://bengelhaupt.com/projects/smartmirror

*/

#include <LEDMatrix.h>
#include <Time.h>
#include <Timezone.h>
#include <dht.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>

///////SETUP

//Refresh intervals in milliseconds
#define MATRIX_REFRESH_INTERVAL           50
#define AMBIENTS_REFRESH_INTERVAL         30000   //30s
#define NTP_SYNC_INTERVAL                 3600000 //1 hour
#define CLIENT_HANDLING_INTERVAL_ENABLED  100
#define CLIENT_HANDLING_INTERVAL_DISABLED 1000

//WiFi
char ssid[] = "YOUR_SSID";       //your network SSID (name)
char pass[] = "YOUR_PASSWORD";   //your network password

//NTP server
const char* ntpServerName = "0.de.pool.ntp.org";

//Additional time settings
TimeChangeRule myDST = {"CEST", Last, Sun, Mar, 2, 120};    //Daylight time = UTC + 2 hours
TimeChangeRule mySTD = {"CET", Last, Sun, Oct, 2, 60};      //Standard time = UTC + 1 hours

//DHT ambients sensor at pin 13
dht DHT;
#define DHTPIN 13

//Matrix size
#define WIDTH   64
#define HEIGHT  16

//LEDMatrix(a, b, c, d, oe, r1, lat, clk);
LEDMatrix matrix(15, 12, 14, 2, 0, 4, 5, 16);

//Default values
#define DEFAULT_CLOCK_TYPE_DECIMAL  false    //whether to show the decimal clock at start
#define DEFAULT_TEXT_STYLE          0       //marquee=0|center=1|leftbound=2|rightbound=3
#define DEFAULT_MARQUEE_SPEED       1000    //delay in milliseconds between shifts
#define DEFAULT_MARQUEE_DIRECTION   false   //right=true|left=false

///////

//Display Buffer 128 = 64 * 16 / 8
uint8_t displaybuf[WIDTH * HEIGHT] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

//Dot font
const uint8_t symbols[] = {
  0x38, 0x44, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x44, 0x38, // 0
  0x02, 0x06, 0x0A, 0x12, 0x22, 0x42, 0x82, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, // 1
  0x38, 0x44, 0x82, 0x82, 0x82, 0x02, 0x02, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x80, 0xFE, // 2
  0x38, 0x44, 0x82, 0x82, 0x02, 0x02, 0x04, 0x18, 0x04, 0x02, 0x02, 0x02, 0x82, 0x82, 0x44, 0x38, // 3
  0x04, 0x0C, 0x0C, 0x14, 0x14, 0x24, 0x24, 0x44, 0x44, 0x84, 0xFE, 0x04, 0x04, 0x04, 0x04, 0x04, // 4
  0xFE, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xF8, 0x04, 0x02, 0x02, 0x02, 0x82, 0x82, 0x44, 0x38, // 5
  0x1C, 0x20, 0x40, 0x80, 0x80, 0x80, 0x80, 0xB8, 0xC4, 0x82, 0x82, 0x82, 0x82, 0x82, 0x44, 0x38, // 6
  0xFE, 0x02, 0x04, 0x04, 0x04, 0x08, 0x08, 0x08, 0x7C, 0x10, 0x10, 0x20, 0x20, 0x20, 0x40, 0x40, // 7
  0x38, 0x44, 0x82, 0x82, 0x82, 0x82, 0x44, 0x38, 0x44, 0x82, 0x82, 0x82, 0x82, 0x82, 0x44, 0x38, // 8
  0x38, 0x44, 0x82, 0x82, 0x82, 0x82, 0x82, 0x46, 0x3A, 0x02, 0x02, 0x02, 0x02, 0x04, 0x08, 0x70, // 9
  0x60, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0s
  0x10, 0x30, 0x50, 0x90, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 1s
  0x60, 0x90, 0x10, 0x10, 0x20, 0x40, 0x80, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 2s
  0x60, 0x90, 0x10, 0x20, 0x10, 0x10, 0x90, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 3s
  0x20, 0x60, 0xA0, 0xA0, 0xF0, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 4s
  0xF0, 0x80, 0x80, 0xE0, 0x10, 0x10, 0x90, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 5s
  0x60, 0x90, 0x80, 0xE0, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 6s
  0xF0, 0x10, 0x20, 0xF0, 0x40, 0x40, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 7s
  0x60, 0x90, 0x90, 0x60, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8s
  0x60, 0x90, 0x90, 0x90, 0x70, 0x10, 0x90, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 9s
  0xE0, 0xA0, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // °s
  0xE0, 0xA4, 0xE8, 0x10, 0x20, 0x5C, 0x94, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // %s (w=6)

  /*   (32) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* ! (33) */ 0x00, 0x00, 0x00, 0x18, 0x3C, 0x3C, 0x3C, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00,
  /* " (34) */ 0x00, 0x00, 0x66, 0x66, 0x66, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* # (35) */ 0x00, 0x00, 0x00, 0x00, 0x6C, 0x6C, 0xFE, 0x6C, 0x6C, 0x6C, 0xFE, 0x6C, 0x6C, 0x00, 0x00, 0x00,
  /* $ (36) */ 0x00, 0x18, 0x18, 0x7C, 0xC6, 0xC2, 0xC0, 0x7C, 0x06, 0x06, 0x86, 0xC6, 0x7C, 0x18, 0x18, 0x00,
  /* % (37) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0xC2, 0xC6, 0x0C, 0x18, 0x30, 0x60, 0xC6, 0x86, 0x00, 0x00, 0x00,
  /* & (38) */ 0x00, 0x00, 0x00, 0x38, 0x6C, 0x6C, 0x38, 0x76, 0xDC, 0xCC, 0xCC, 0xCC, 0x76, 0x00, 0x00, 0x00,
  /* ' (39) */ 0x00, 0x00, 0x30, 0x30, 0x30, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* ( (40) */ 0x00, 0x00, 0x00, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x00, 0x00, 0x00,
  /* ) (41) */ 0x00, 0x00, 0x00, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00, 0x00, 0x00,
  /* * (42) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* + (43) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* , (44) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0x30, 0x00, 0x00,
  /* - (45) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* . (46) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00,
  /* / (47) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x80, 0x00, 0x00, 0x00,
  /* 0 (48) */ 0x00, 0x00, 0x00, 0x3C, 0x66, 0xC3, 0xC3, 0xDB, 0xDB, 0xC3, 0xC3, 0x66, 0x3C, 0x00, 0x00, 0x00,
  /* 1 (49) */ 0x00, 0x00, 0x00, 0x18, 0x38, 0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00, 0x00, 0x00,
  /* 2 (50) */ 0x00, 0x00, 0x00, 0x7C, 0xC6, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0xC6, 0xFE, 0x00, 0x00, 0x00,
  /* 3 (51) */ 0x00, 0x00, 0x00, 0x7C, 0xC6, 0x06, 0x06, 0x3C, 0x06, 0x06, 0x06, 0xC6, 0x7C, 0x00, 0x00, 0x00,
  /* 4 (52) */ 0x00, 0x00, 0x00, 0x0C, 0x1C, 0x3C, 0x6C, 0xCC, 0xFE, 0x0C, 0x0C, 0x0C, 0x1E, 0x00, 0x00, 0x00,
  /* 5 (53) */ 0x00, 0x00, 0x00, 0xFE, 0xC0, 0xC0, 0xC0, 0xFC, 0x06, 0x06, 0x06, 0xC6, 0x7C, 0x00, 0x00, 0x00,
  /* 6 (54) */ 0x00, 0x00, 0x00, 0x38, 0x60, 0xC0, 0xC0, 0xFC, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00, 0x00, 0x00,
  /* 7 (55) */ 0x00, 0x00, 0x00, 0xFE, 0xC6, 0x06, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00,
  /* 8 (56) */ 0x00, 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0x7C, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00, 0x00, 0x00,
  /* 9 (57) */ 0x00, 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0x7E, 0x06, 0x06, 0x06, 0x0C, 0x78, 0x00, 0x00, 0x00,
  /* : (58) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
  /* ; (59) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30, 0x00, 0x00, 0x00,
  /* < (60) */ 0x00, 0x00, 0x00, 0x00, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x00, 0x00, 0x00,
  /* = (61) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* > (62) */ 0x00, 0x00, 0x00, 0x00, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x00, 0x00, 0x00,
  /* ? (63) */ 0x00, 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0x0C, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00,
  /* @ (64) */ 0x00, 0x00, 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xDE, 0xDE, 0xDE, 0xDC, 0xC0, 0x7C, 0x00, 0x00, 0x00,
  /* A (65) */ 0x00, 0x00, 0x00, 0x10, 0x38, 0x6C, 0xC6, 0xC6, 0xFE, 0xC6, 0xC6, 0xC6, 0xC6, 0x00, 0x00, 0x00,
  /* B (66) */ 0x00, 0x00, 0x00, 0xFC, 0x66, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x66, 0x66, 0xFC, 0x00, 0x00, 0x00,
  /* C (67) */ 0x00, 0x00, 0x00, 0x3C, 0x66, 0xC2, 0xC0, 0xC0, 0xC0, 0xC0, 0xC2, 0x66, 0x3C, 0x00, 0x00, 0x00,
  /* D (68) */ 0x00, 0x00, 0x00, 0xF8, 0x6C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x6C, 0xF8, 0x00, 0x00, 0x00,
  /* E (69) */ 0x00, 0x00, 0x00, 0xFE, 0x66, 0x62, 0x68, 0x78, 0x68, 0x60, 0x62, 0x66, 0xFE, 0x00, 0x00, 0x00,
  /* F (70) */ 0x00, 0x00, 0x00, 0xFE, 0x66, 0x62, 0x68, 0x78, 0x68, 0x60, 0x60, 0x60, 0xF0, 0x00, 0x00, 0x00,
  /* G (71) */ 0x00, 0x00, 0x00, 0x3C, 0x66, 0xC2, 0xC0, 0xC0, 0xDE, 0xC6, 0xC6, 0x66, 0x3A, 0x00, 0x00, 0x00,
  /* H (72) */ 0x00, 0x00, 0x00, 0xC6, 0xC6, 0xC6, 0xC6, 0xFE, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x00, 0x00, 0x00,
  /* I (73) */ 0x00, 0x00, 0x00, 0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00,
  /* J (74) */ 0x00, 0x00, 0x00, 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0xCC, 0xCC, 0xCC, 0x78, 0x00, 0x00, 0x00,
  /* K (75) */ 0x00, 0x00, 0x00, 0xE6, 0x66, 0x66, 0x6C, 0x78, 0x78, 0x6C, 0x66, 0x66, 0xE6, 0x00, 0x00, 0x00,
  /* L (76) */ 0x00, 0x00, 0x00, 0xF0, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x62, 0x66, 0xFE, 0x00, 0x00, 0x00,
  /* M (77) */ 0x00, 0x00, 0x00, 0xC3, 0xE7, 0xFF, 0xFF, 0xDB, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x00, 0x00, 0x00,
  /* N (78) */ 0x00, 0x00, 0x00, 0xC6, 0xE6, 0xF6, 0xFE, 0xDE, 0xCE, 0xC6, 0xC6, 0xC6, 0xC6, 0x00, 0x00, 0x00,
  /* O (79) */ 0x00, 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00, 0x00, 0x00,
  /* P (80) */ 0x00, 0x00, 0x00, 0xFC, 0x66, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x60, 0xF0, 0x00, 0x00, 0x00,
  /* Q (81) */ 0x00, 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xD6, 0xDE, 0x7C, 0x0C, 0x0E, 0x00,
  /* R (82) */ 0x00, 0x00, 0x00, 0xFC, 0x66, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0x66, 0x66, 0xE6, 0x00, 0x00, 0x00,
  /* S (83) */ 0x00, 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0x60, 0x38, 0x0C, 0x06, 0xC6, 0xC6, 0x7C, 0x00, 0x00, 0x00,
  /* T (84) */ 0x00, 0x00, 0x00, 0xFF, 0xDB, 0x99, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00,
  /* U (85) */ 0x00, 0x00, 0x00, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00, 0x00, 0x00,
  /* V (86) */ 0x00, 0x00, 0x00, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x66, 0x3C, 0x18, 0x00, 0x00, 0x00,
  /* W (87) */ 0x00, 0x00, 0x00, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xDB, 0xDB, 0xFF, 0x66, 0x66, 0x00, 0x00, 0x00,
  /* X (88) */ 0x00, 0x00, 0x00, 0xC3, 0xC3, 0x66, 0x3C, 0x18, 0x18, 0x3C, 0x66, 0xC3, 0xC3, 0x00, 0x00, 0x00,
  /* Y (89) */ 0x00, 0x00, 0x00, 0xC3, 0xC3, 0xC3, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00,
  /* Z (90) */ 0x00, 0x00, 0x00, 0xFF, 0xC3, 0x86, 0x0C, 0x18, 0x30, 0x60, 0xC1, 0xC3, 0xFF, 0x00, 0x00, 0x00,
  /* [ (91) */ 0x00, 0x00, 0x00, 0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00, 0x00, 0x00,
  /* \ (92) */ 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0x70, 0x38, 0x1C, 0x0E, 0x06, 0x02, 0x00, 0x00, 0x00,
  /* ] (93) */ 0x00, 0x00, 0x00, 0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3C, 0x00, 0x00, 0x00,
  /* ^ (94) */ 0x00, 0x10, 0x38, 0x6C, 0xC6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* _ (95) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,
  /* ` (96) */ 0x00, 0x30, 0x30, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* a (97) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x0C, 0x7C, 0xCC, 0xCC, 0xCC, 0x76, 0x00, 0x00, 0x00,
  /* b (98) */ 0x00, 0x00, 0x00, 0xE0, 0x60, 0x60, 0x78, 0x6C, 0x66, 0x66, 0x66, 0x66, 0x7C, 0x00, 0x00, 0x00,
  /* c (99) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0xC6, 0xC0, 0xC0, 0xC0, 0xC6, 0x7C, 0x00, 0x00, 0x00,
  /* d (100) */ 0x00, 0x00, 0x00, 0x1C, 0x0C, 0x0C, 0x3C, 0x6C, 0xCC, 0xCC, 0xCC, 0xCC, 0x76, 0x00, 0x00, 0x00,
  /* e (101) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0xC6, 0xFE, 0xC0, 0xC0, 0xC6, 0x7C, 0x00, 0x00, 0x00,
  /* f (102) */ 0x00, 0x00, 0x00, 0x38, 0x6C, 0x64, 0x60, 0xF0, 0x60, 0x60, 0x60, 0x60, 0xF0, 0x00, 0x00, 0x00,
  /* g (103) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x7C, 0x0C, 0xCC, 0x78,
  /* h (104) */ 0x00, 0x00, 0x00, 0xE0, 0x60, 0x60, 0x6C, 0x76, 0x66, 0x66, 0x66, 0x66, 0xE6, 0x00, 0x00, 0x00,
  /* i (105) */ 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00,
  /* j (106) */ 0x00, 0x00, 0x00, 0x06, 0x06, 0x00, 0x0E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x66, 0x66, 0x3C,
  /* k (107) */ 0x00, 0x00, 0x00, 0xE0, 0x60, 0x60, 0x66, 0x6C, 0x78, 0x78, 0x6C, 0x66, 0xE6, 0x00, 0x00, 0x00,
  /* l (108) */ 0x00, 0x00, 0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00,
  /* m (109) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE6, 0xFF, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0x00, 0x00, 0x00,
  /* n (110) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xDC, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00,
  /* o (111) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00, 0x00, 0x00,
  /* p (112) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xDC, 0x66, 0x66, 0x66, 0x66, 0x66, 0x7C, 0x60, 0x60, 0xF0,
  /* q (113) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x7C, 0x0C, 0x0C, 0x1E,
  /* r (114) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xDC, 0x76, 0x66, 0x60, 0x60, 0x60, 0xF0, 0x00, 0x00, 0x00,
  /* s (115) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0xC6, 0x60, 0x38, 0x0C, 0xC6, 0x7C, 0x00, 0x00, 0x00,
  /* t (116) */ 0x00, 0x00, 0x00, 0x10, 0x30, 0x30, 0xFC, 0x30, 0x30, 0x30, 0x30, 0x36, 0x1C, 0x00, 0x00, 0x00,
  /* u (117) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x76, 0x00, 0x00, 0x00,
  /* v (118) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC3, 0xC3, 0xC3, 0xC3, 0x66, 0x3C, 0x18, 0x00, 0x00, 0x00,
  /* w (119) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC3, 0xC3, 0xC3, 0xDB, 0xDB, 0xFF, 0x66, 0x00, 0x00, 0x00,
  /* x (120) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC3, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0xC3, 0x00, 0x00, 0x00,
  /* y (121) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7E, 0x06, 0x0C, 0xF8,
  /* z (122) */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xCC, 0x18, 0x30, 0x60, 0xC6, 0xFE, 0x00, 0x00, 0x00,
  /* { (123) */ 0x00, 0x00, 0x00, 0x0E, 0x18, 0x18, 0x18, 0x70, 0x18, 0x18, 0x18, 0x18, 0x0E, 0x00, 0x00, 0x00,
  /* | (124) */ 0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00,
  /* } (125) */ 0x00, 0x00, 0x00, 0x70, 0x18, 0x18, 0x18, 0x0E, 0x18, 0x18, 0x18, 0x18, 0x70, 0x00, 0x00, 0x00,
  /* ~ (126) */ 0x00, 0x00, 0x00, 0x76, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* BLK(127)*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

boolean enabled = true;
boolean decimalClock = DEFAULT_CLOCK_TYPE_DECIMAL;
String text = "";
uint8_t textStyle = DEFAULT_TEXT_STYLE;
int marqueeSpeed = DEFAULT_MARQUEE_SPEED;
boolean marqueeDirection = DEFAULT_MARQUEE_DIRECTION;

ESP8266WebServer server(80);

WiFiUDP udp;
unsigned int localPort = 2390;
IPAddress timeServerIP;
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[ NTP_PACKET_SIZE];

//(x, y) top-left position
void drawSymbol(uint16_t x, uint16_t y, uint8_t n) {
  if (x % 8 != 0)
    for (uint8_t i = 0; i < HEIGHT; i++) {
      uint8_t shift = x % 8;
      displaybuf[(y + i) * (WIDTH / 8) + (x / 8)] |= symbols[(n * 16) + i] >> shift;
      displaybuf[(y + i) * (WIDTH / 8) + (x / 8) + 1] |= symbols[(n * 16) + i] << (8 - shift);
    }
  else
    for (uint8_t i = 0; i < 16; i++) {
      displaybuf[(y + i) * (WIDTH / 8) + (x / 8)] = 0x00;
      displaybuf[(y + i) * (WIDTH / 8) + (x / 8)] = symbols[(n * 16) + i];
    }
}

//TIME
void drawTimeDecimal(uint8_t hour, uint8_t minute, uint8_t second) {
  if (hour > 9 || minute > 99 || second > 99)
    return;

  matrix.clear();

  drawSymbol(1, 0, hour);

  matrix.drawPoint(9, 5, 1);
  matrix.drawPoint(9, 11, 1);

  drawSymbol(11, 0, minute / 10);
  drawSymbol(19, 0, minute % 10);

  matrix.drawPoint(27, 5, 1);
  matrix.drawPoint(27, 11, 1);

  drawSymbol(29, 0, second / 10);
  drawSymbol(37, 0, second % 10);
}

void drawTime(uint8_t hour, uint8_t minute, uint8_t second) {
  if (hour > 23 || minute > 59 || second > 59)
    return;

  matrix.clear();

  drawSymbol(0, 0, hour / 10);
  drawSymbol(7, 0, hour % 10);

  matrix.drawPoint(15, 5, 1);
  matrix.drawPoint(15, 11, 1);

  drawSymbol(17, 0, minute / 10);
  drawSymbol(24, 0, minute % 10);

  matrix.drawPoint(32, 5, 1);
  matrix.drawPoint(32, 11, 1);

  drawSymbol(34, 0, second / 10);
  drawSymbol(41, 0, second % 10);
}

uint16_t startmillisoffset = 0;
long lastSync = millis();
long timeSyncInterval = 10000;
void syncTime() {
  WiFi.hostByName(ntpServerName, timeServerIP);
  sendNTPpacket(timeServerIP);
  delay(1000);

  int cb = udp.parsePacket();
  if (!cb && WiFi.status() == WL_CONNECTED) {
    timeSyncInterval = 10000;
  }
  else {
    udp.read(packetBuffer, NTP_PACKET_SIZE);

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    unsigned long epoch = secsSince1900 - 2208988800UL;

    Timezone myTZ(myDST, mySTD);

    time_t localTime = myTZ.toLocal(epoch + 2);

    setTime(hour(localTime), minute(localTime), second(localTime), day(localTime), month(localTime), year(localTime));
    startmillisoffset = getMillisPart(millis());
    refresh();
    timeSyncInterval = NTP_SYNC_INTERVAL;
    lastSync = millis();
    Serial.println("Time synced");
  }
}

unsigned long sendNTPpacket(IPAddress& address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;       // LI, Version, Mode
  packetBuffer[1] = 0;                // Stratum, or type of clock
  packetBuffer[2] = 6;                // Polling Interval
  packetBuffer[3] = 0xEC;             // Peer Clock Precision
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  udp.beginPacket(address, 123);
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

uint16_t getMillisPart(long millis) {
  return millis % 1000;
}

//AMBIENTS
int lastTemperatureValue = 0;
int lastHumidityValue = 0;
long lastAmbientRefresh = millis();

void refreshAmbients() {
  if (DHT.read22(DHTPIN) == DHTLIB_OK) {
    lastHumidityValue = round(DHT.humidity);
    lastTemperatureValue = round(DHT.temperature);
  }

  Serial.println("Ambients refreshed");
  lastAmbientRefresh = millis();
}

void drawAmbientsDecimal() {
  //draw temperature
  if (lastTemperatureValue > 9)
    drawSymbol(47, 0, lastTemperatureValue / 10 + 10);
  drawSymbol(52, 0, lastTemperatureValue % 10 + 10);
  drawSymbol(57, 0, 20);

  //draw humidity
  if (lastHumidityValue > 9)
    drawSymbol(47, 8, lastHumidityValue / 10 + 10);
  drawSymbol(52, 8, lastHumidityValue % 10 + 10);
  drawSymbol(57, 8, 21);
}

void drawAmbients() {
  //draw temperature
  if (lastTemperatureValue > 9)
    drawSymbol(49, 0, lastTemperatureValue / 10 + 10);
  drawSymbol(53, 0, lastTemperatureValue % 10 + 10);
  drawSymbol(58, 0, 20);

  //draw humidity
  if (lastHumidityValue > 9)
    drawSymbol(49, 8, lastHumidityValue / 10 + 10);
  drawSymbol(53, 8, lastHumidityValue % 10 + 10);
  drawSymbol(58, 8, 21);
}

long lastMarquee = millis();
long marqueeCount = 0;
void drawText() {
  int len = text.length();
  switch (textStyle) {
    case 0:
      if (millis() - lastMarquee > marqueeSpeed) {
        matrix.clear();
        for (int i = 0; i < 8; i++)
          drawSymbol(8 * i, 0, text[i] - 10);

        if (marqueeDirection)
          text = text[len - 1] + text.substring(0, text.length() - 1);
        else
          text = text.substring(1, text.length()) + text[0];
        lastMarquee = millis();
      }
      break;
    case 1:
      matrix.clear();
      for (int i = 0; i < len; i++) {
        int xpos = 32 - 4 * len + 8 * i;
        if (xpos > -8 && xpos < 64)
          drawSymbol(xpos, 0, text[i] - 10);
      }
      break;
    case 2:
      matrix.clear();
      for (int i = 0; i < len; i++) {
        int xpos = 8 * i + 1;
        if (xpos > -8 && xpos < 64)
          drawSymbol(xpos, 0, text[i] - 10);
      }
      break;
    case 3:
      matrix.clear();
      for (int i = 0; i < len; i++) {
        int xpos = 64 - 8 * len + 8 * i;
        if (xpos > -8 && xpos < 64)
          drawSymbol(xpos, 0, text[i] - 10);
      }
      break;
  }
}

long lastMatrixRefresh = millis();
void refresh() {
  int16_t offsetmillis = getMillisPart(millis()) - startmillisoffset;
  time_t n = now();

  if (decimalClock) {
    long daymillis = 3600000.0 * hour(n) + 60000.0 * minute(n) + 1000.0 * second(n) + offsetmillis;
    if (offsetmillis < 0)
      daymillis += 1000;
    double t = daymillis / 8640000.0;
    uint8_t h = t;
    uint8_t m = 100.0 * (t - h);
    uint8_t s = 100.0 * ((t - h) * 100.0 - m);
    drawTimeDecimal(h, m, s);
    drawAmbientsDecimal();
  }
  else {
    drawTime(hour(n), minute(n), second(n));
    drawAmbients();
  }

  lastMatrixRefresh = millis();
}

long lastServerClientHandling = millis();
void serverHandleClient() {
  server.handleClient();
  lastServerClientHandling = millis();
}

//SETUP/LOOP
long lastWiFiCheck = millis();
uint8_t loadingState = 1;
void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  matrix.begin(displaybuf, WIDTH, HEIGHT);
  matrix.clear();

  //init WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (true) {
    if (millis() - lastWiFiCheck > 300) {
      if (WiFi.status() == WL_CONNECTED)
        break;
      Serial.print(".");
      matrix.drawRect(28, 4, 36, 12, loadingState);
      if (loadingState == 0)
        loadingState = 1;
      else
        loadingState = 0;

      lastWiFiCheck = millis();
    }
    matrix.scan();
    yield();
  }
  matrix.clear();

  Serial.println("WiFi connected");

  server.on("/", []() {
    server.send(200, "text/html", "See request syntax on <a href=\"http://bensoft.de/projects/smartmirror/\">http://bensoft.de/projects/smartmirror/</a>");
  });
  server.on("/temperature", []() {
    refreshAmbients();
    server.send(200, "text/html", (String)lastTemperatureValue);
  });
  server.on("/humidity", []() {
    refreshAmbients();
    server.send(200, "text/html", (String)lastHumidityValue);
  });
  server.on("/enable", []() {
    enabled = true;
    matrix.on();
    server.send(200, "text/html", "enabled");
  });
  server.on("/disable", []() {
    enabled = false;
    matrix.off();
    server.send(200, "text/html", "disabled");
  });
  server.on("/text", []() {
    String message = "";
    if (server.arg("t") == "") {  //Parameter not found
      message = "no text (argument 't') given or text empty";
    } else {   //Parameter found
      message = "showing text";
      text = server.arg("t");
      if (server.arg("style") != "") {
        String style = server.arg("style");
        if (style == "marquee" || style == "center" || style == "rightbound" || style == "leftbound") {
          message += " with style " + style;
          if (style == "marquee") {
            String marqSpeed = server.arg("speed");
            if (marqSpeed != "")
              marqueeSpeed = atoi(marqSpeed.c_str());
            else
              marqueeSpeed = DEFAULT_MARQUEE_SPEED;

            String marqDir = server.arg("direction");
            if (marqDir != "") {
              if (marqDir == "right")
                marqueeDirection = true;
              if (marqDir == "left")
                marqueeDirection = false;
            }
            else {
              marqueeDirection = DEFAULT_MARQUEE_DIRECTION;
            }
            textStyle = 0;
            marqueeCount = 0;
          }
          if (style == "center")
            textStyle = 1;
          if (style == "leftbound")
            textStyle = 2;
          if (style == "rightbound")
            textStyle = 3;
        }
        else {
          message += " (style argument has invalid value)";
        }
      }
      else {
        textStyle = DEFAULT_TEXT_STYLE;
      }
    }
    server.send(200, "text/html", message);
  });
  server.on("/clock24", []() {
    text = "";
    decimalClock = false;
    server.send(200, "text/html", "showing 24-hour clock");
  });
  server.on("/clock10", []() {
    text = "";
    decimalClock = true;
    server.send(200, "text/html", "showing decimal clock");
  });
  server.begin();

  udp.begin(localPort);

  setTime(0, 0, 0, 1, 1, 2000);

  syncTime();

  refreshAmbients(); //refresh ambients
}

void loop() {
  long currentMillis = millis();
  if (enabled) {
    if (text == "") {
      if (currentMillis - lastSync > timeSyncInterval)
        syncTime();

      if (currentMillis - lastMatrixRefresh > MATRIX_REFRESH_INTERVAL)
        refresh();

      if (currentMillis - lastAmbientRefresh > AMBIENTS_REFRESH_INTERVAL)
        refreshAmbients();
    }
    else {
      drawText();
    }

    matrix.scan();

    if (currentMillis - lastServerClientHandling > CLIENT_HANDLING_INTERVAL_ENABLED)
      serverHandleClient();
  }
  else {
    if (currentMillis - lastServerClientHandling > CLIENT_HANDLING_INTERVAL_DISABLED)
      serverHandleClient();
  }
}
