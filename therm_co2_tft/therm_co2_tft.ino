#include <cstddef>

#include "Adafruit_MCP9808.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>
#include <SensirionI2CScd4x.h>

#define BLYNK_PRINT Serial
#define BLYNK_DEBUG

#define TFT_CS         0
#define TFT_RST        -1  // RST pin instead
#define TFT_DC         2
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

char auth[] = "XXXXXXXX";
char ssid[] = "XXXXXXXX";
char pass[] = "XXXXXXXX";

SensirionI2CScd4x scd4x;
BlynkTimer timer;

//    0                                 320
//  0 ┌───────────────────────────────────┐
//    │                                   │
//    │                                   │
//    │              (GRAPH)              │
//    │                                   │
//    │                                   │
// 200├───────────────────────────────────┤
//    │                                   │
//    │ (GRAPH DURATION)   (CURRENT TEMP) │
//    │                                   │
// 239└───────────────────────────────────┘
#define GRAPHTOP        0
#define GRAPHBOTTOM     200
#define GRAPHWIDTH      320
#define TEXTTOP         207
#define TEXTLEFT        5
#define TEXTRIGHT       314
#define CO2MAX          2500
#define COLOR_TEMP      0x0018
#define COLOR_CO2       0xc000
#define COLOR_BOTH      (COLOR_TEMP | COLOR_CO2)

int graphDurs[3] = {2, 24, 72};  // Hours
int showGraph = 0;

struct meas {
  float tempF;
  uint16_t co2;
  float humidity;
};

// Used as ring buffers.
struct meas measHist[3][GRAPHWIDTH];
size_t histIdx[3] = {0,0,0};
unsigned long lastMeasMillis[3] = {0, 0, 0};

void setup() {
  Serial.begin(9600);
  while (!Serial); //waits for serial terminal to be open, necessary in newer arduino boards.
  Serial.println("Initializing..");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off

  Wire.begin();
  scd4x.begin(Wire);
  uint16_t error = scd4xReinit();
  if (error) {
    char errorMessage[256];
    Serial.print("Error trying to execute startPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  Serial.println("Initializing ST7789 TFT");
  tft.init(240, 320);
  // 3=pins at right, 1=pins at left
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.print("init...");

  Serial.println("Connecting to Blynk..");
  Blynk.begin(auth, ssid, pass);
  timer.setInterval((graphDurs[0] * 3600000L) / GRAPHWIDTH, blynkTempEvent);
  Serial.println("Ready");

  // Because Blynktimer doesn't run immediately.
  blynkTempEvent();
}

uint16_t scd4xReinit() {
  scd4x.stopPeriodicMeasurement();
  delay(1000);
  return scd4x.startPeriodicMeasurement();
}

void blynkTempEvent() {
  uint16_t error;
  struct meas nowMeas;
  float tempC;
  digitalWrite(LED_BUILTIN, LOW);
  error = scd4x.readMeasurement(nowMeas.co2, tempC, nowMeas.humidity);
  if (error) {
    // Reinit and try again
    error = scd4xReinit();
    if (!error)
      error = scd4x.readMeasurement(nowMeas.co2, tempC, nowMeas.humidity);
  }
  if (error) {
    char errorMessage[256];
    Serial.print("readMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    tft.setCursor(0, 0);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.print(errorMessage);
  } else {
    nowMeas.tempF = (9.0 / 5) * tempC + 32.0;
    Serial.print("Temp: "); 
    Serial.print(nowMeas.tempF, 4);
    Serial.print(" F, CO2: "); 
    Serial.print(nowMeas.co2);
    Serial.println(" ppm");
    saveMeas(nowMeas);
    updateTFT(nowMeas);
  }
  digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off
}

void saveMeas(struct meas nowMeas) {
  // Save to Blynk
  Blynk.virtualWrite(V5, nowMeas.tempF);
  Blynk.virtualWrite(V7, nowMeas.co2);
  
  // Save in each of the ring buffers if it's been long enough since the last measurement.
  unsigned long now = millis();
  for (int i=0; i<3; i++) {
    unsigned long interval = (graphDurs[i] * 3600000L) / GRAPHWIDTH;
    if (now - lastMeasMillis[i] < interval) {
      continue;
    }
    lastMeasMillis[i] = now;
    measHist[i][histIdx[i]] = nowMeas;
    histIdx[i] = (histIdx[i] + 1) % GRAPHWIDTH;
  }
}

void updateTFT(struct meas nowMeas) {  
  // Figure out the min and max;
  float minF = 9999;
  float maxF = -9999;
  for (size_t n = 0; n < GRAPHWIDTH; n++) {
    float t = measHist[showGraph][n].tempF;
    if (t != 0) {
      if (t < minF) minF = t;
      if (t > maxF) maxF = t;
    }
  }

  // Make sure the y axis is at least 1.0.
  float diff = maxF - minF;
  if (diff < 1.0) {
    maxF += (1.0 - diff)/2;
    minF -= (1.0 - diff)/2;
  }

  // Measure size of labels
  tft.setTextSize(3);
  uint16_t durWidth, tempWidth, co2Width;
  char tempStr[6], co2Str[6], durStr[3];
  dtostrf(nowMeas.tempF, 0, 1, tempStr);
  {
    int16_t x, y;
    uint16_t w1, w2, h;
    tft.getTextBounds(tempStr, 0, 0, &x, &y, &w1, &h);
    tft.getTextBounds("F", 0, 0, &x, &y, &w2, &h);
    tempWidth = w1+w2;
  }
  dtostrf(nowMeas.co2, 0, 0, co2Str);
  {
    int16_t x, y;
    uint16_t w1, w2, h;
    tft.getTextBounds(co2Str, 0, 0, &x, &y, &w1, &h);
    tft.getTextBounds("ppm", 0, 0, &x, &y, &w2, &h);
    co2Width = w1+w2;
  }
  dtostrf(graphDurs[showGraph], 0, 0, durStr);
  {
    int16_t x, y;
    uint16_t w1, w2, h;
    tft.getTextBounds(durStr, 0, 0, &x, &y, &w1, &h);
    tft.getTextBounds("h", 0, 0, &x, &y, &w2, &h);
    durWidth = w1+w2;
  }

  // Clear screen
  tft.fillScreen(ST77XX_BLACK);

  // Graph duration
  tft.setCursor(TEXTLEFT, TEXTTOP);
  tft.setTextColor(0x738e);
  tft.print(graphDurs[showGraph]);
  tft.print("h");

  // Current measurements
  tft.setCursor(TEXTRIGHT-tempWidth, TEXTTOP);
  tft.setTextColor(COLOR_TEMP);
  tft.print(tempStr);
  tft.print("F");
  tft.setCursor((durWidth+TEXTRIGHT-tempWidth-co2Width)/2, TEXTTOP);
  tft.setTextColor(COLOR_CO2);
  tft.print(co2Str);
  tft.print("ppm");

  // Print a graph, scaled to the min and max.
  for (size_t n = 0; n < GRAPHWIDTH; n++) {
    struct meas m = measHist[showGraph][(histIdx[showGraph] + n) % GRAPHWIDTH];
    int tempH = 2;
    if (m.tempF > minF) {
      tempH = (m.tempF - minF) / (maxF - minF) * (GRAPHBOTTOM-GRAPHTOP);
    }
    int co2H = float(m.co2) / float(CO2MAX) * (GRAPHBOTTOM-GRAPHTOP);

    // Draw overlap
    int minH = tempH > co2H ? co2H : tempH;
    tft.drawLine(n, GRAPHBOTTOM-minH, n, GRAPHBOTTOM, COLOR_BOTH);
    // Draw whichever is higher.
    if (tempH > co2H) {
      tft.drawLine(n, GRAPHBOTTOM-tempH, n, GRAPHBOTTOM-minH-1, COLOR_TEMP);
    } else {
      tft.drawLine(n, GRAPHBOTTOM-co2H, n, GRAPHBOTTOM-minH-1, COLOR_CO2);
    }
 
    // Brighter lines at whole degrees
    for (float s = floor(m.tempF); s > minF; s--) {
      int sh = (s - minF) / (maxF - minF) * (GRAPHBOTTOM-GRAPHTOP);
      if (sh > minH) {
        tft.drawLine(n, GRAPHBOTTOM-sh, n, GRAPHBOTTOM-sh, 0x0398);
      }
    }
  }
  showGraph = (showGraph + 1) % 3;
}

void loop() {
  Blynk.run();
  timer.run();
}
