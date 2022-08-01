#include <cstddef>

#include "Adafruit_MCP9808.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>

#define BLYNK_PRINT Serial
#define BLYNK_DEBUG

#define TFT_CS         0
#define TFT_RST        -1  // RST pin instead
#define TFT_DC         2
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

char auth[] = "XXXXXXXX";
char ssid[] = "XXXXXXXX";
char pass[] = "XXXXXXXX";

Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();
BlynkTimer timer;

//    0                                 239
//  0 ┌───────────────────────────────────┐
//    │                                   │
//    │                                   │
//    │              (GRAPH)              │
//    │                                   │
//    │                                   │
// 100├───────────────────────────────────┤
//    │                                   │
//    │ (GRAPH DURATION)   (CURRENT TEMP) │
//    │                                   │
// 134└───────────────────────────────────┘
#define TEMPHISTTOP        0
#define TEMPHISTBOTTOM     100
#define TEMPHISTWIDTH      240
#define TEXTTOP            105
#define TEXTLEFT           5
#define TEXTRIGHT          234
int graphDurs[3] = {1, 24, 72};  // Hours
int showGraph = 0;

// Used as ring buffers.
float temps[3][TEMPHISTWIDTH];
size_t tempIdx[3] = {0,0,0};
unsigned long lastTempMillis[3] = {0, 0, 0};

void setup() {
  Serial.begin(9600);
  while (!Serial); //waits for serial terminal to be open, necessary in newer arduino boards.
  Serial.println("Initializing..");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off

  if (!tempsensor.begin(0x18)) {
    Serial.println("Couldn't find MCP9808! Check your connections and verify the address is correct.");
    while (1);
  }
  Serial.println("Found MCP9808!");
  tempsensor.setResolution(3);  // 0.0625°C, 250 ms

  Serial.println("Initializing 1.14\" 240x135 TFT");
  tft.init(135, 240);
  tft.fillScreen(ST77XX_BLACK);
  // 3=pins at bottom, 1=pins at top
  tft.setRotation(3);

  Serial.println("Connecting to Blynk..");
  Blynk.begin(auth, ssid, pass);
  timer.setInterval((graphDurs[0] * 3600000L) / TEMPHISTWIDTH, blynkTempEvent);
  Serial.println("Ready");

  // Because Blynktimer doesn't run immediately.
  blynkTempEvent();
}

void blynkTempEvent() {
  Serial.println("wake up MCP9808.... "); // wake up MCP9808 - power consumption ~200 mikro Ampere
  digitalWrite(LED_BUILTIN, LOW);
  tempsensor.wake();   // wake up, ready to read!

  // Read and print out the temperature, also shows the resolution mode used for reading.
  Serial.print("Resolution in mode: ");
  Serial.println (tempsensor.getResolution());
  float tempF = tempsensor.readTempF();
  Serial.print("Temp: "); 
  Serial.print(tempF, 4);
  Serial.println(" F");
  saveTemp(tempF);
  updateTFT(tempF);
  
  delay(200);
  Serial.println("Shutdown MCP9808.... ");
  tempsensor.shutdown_wake(1); // shutdown MSP9808 - power consumption ~0.1 mikro Ampere, stops temperature sampling
  digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off
}

void saveTemp(float tempF) {
  // Save to Blynk
  Blynk.virtualWrite(V5, tempF);
  
  // Save in each of the ring buffers if it's been long enough since the last measurement.
  unsigned long now = millis();
  for (int i=0; i<3; i++) {
    unsigned long interval = (graphDurs[i] * 3600000L) / TEMPHISTWIDTH;
    if (now - lastTempMillis[i] < interval) {
      continue;
    }
    lastTempMillis[i] = now;
    temps[i][tempIdx[i]] = tempF;
    tempIdx[i] = (tempIdx[i] + 1) % TEMPHISTWIDTH;
  }
}

void updateTFT(float tempF) {  
  // Figure out the min and max;
  float minF = 9999;
  float maxF = -9999;
  for (size_t n = 0; n < TEMPHISTWIDTH; n++) {
    float t = temps[showGraph][n];
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

  // Measure size of the current temp
  char tempStr[6];
  dtostrf(tempF, 0, 1, tempStr);
  tft.setTextSize(3);
  int16_t x, y;
  uint16_t w1, w2, h;
  tft.getTextBounds(tempStr, 0, 0, &x, &y, &w1, &h);
  tft.getTextBounds("F", 0, 0, &x, &y, &w2, &h);

  // Clear screen
  tft.fillScreen(ST77XX_BLACK);

  // Current temperature
  tft.setCursor(TEXTRIGHT-w1-w2-5, TEXTTOP);
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(3);
  tft.print(tempStr);
  tft.setTextSize(1);
  tft.print("O");
  tft.setTextSize(3);
  tft.print("F");

  // Graph duration
  tft.setCursor(TEXTLEFT, TEXTTOP+5);
  tft.setTextColor(0x738e);
  tft.setTextSize(2);
  tft.print("last ");
  tft.print(graphDurs[showGraph]);
  tft.print("hr");

  // Print a graph, scaled to the min and max.
  for (size_t n = 0; n < TEMPHISTWIDTH; n++) {
    float t = temps[showGraph][(tempIdx[showGraph] + n) % TEMPHISTWIDTH];
    int h = 2;
    if (t > minF) {
      h = (t - minF) / (maxF - minF) * (TEMPHISTBOTTOM-TEMPHISTTOP);
    }
    tft.drawLine(n, TEMPHISTBOTTOM-h, n, TEMPHISTBOTTOM, 0x0018);
    // Brighter lines at whole degrees
    for (float s = floor(t); s > minF; s--) {
      int sh = (s - minF) / (maxF - minF) * (TEMPHISTBOTTOM-TEMPHISTTOP);
      tft.drawLine(n, TEMPHISTBOTTOM-sh, n, TEMPHISTBOTTOM-sh, 0x0398);
    }
  }
  showGraph = (showGraph + 1) % 3;
}

void loop() {
  Blynk.run();
  timer.run();
}
