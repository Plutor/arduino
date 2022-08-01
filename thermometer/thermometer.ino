
#include <ESP8266WiFi.h>
#include <Wire.h>
#include "Adafruit_MCP9808.h"

#include <BlynkSimpleEsp8266.h>

char auth[] = "XXXXXXXX";
char ssid[] = "XXXXXXXX";
char pass[] = "XXXXXXXX";
int interval_sec = 60;

Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();
BlynkTimer timer;

void setup() {
  Serial.begin(9600);
  while (!Serial); //waits for serial terminal to be open, necessary in newer arduino boards.
  Serial.println("MCP9808 demo");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off

  if (!tempsensor.begin(0x18)) {
    Serial.println("Couldn't find MCP9808! Check your connections and verify the address is correct.");
    while (1);
  }
    
  Serial.println("Found MCP9808!");
  tempsensor.setResolution(3);  // 0.0625°C, 250 ms

  Serial.println("Connecting to Blynk..");
  Blynk.begin(auth, ssid, pass);
  timer.setInterval(interval_sec * 1000L, blynkTempEvent);
  Serial.println("Ready");
}

void blynkTempEvent() {
  Serial.println("wake up MCP9808.... "); // wake up MCP9808 - power consumption ~200 mikro Ampere
  digitalWrite(LED_BUILTIN, LOW);
  tempsensor.wake();   // wake up, ready to read!

  // Read and print out the temperature, also shows the resolution mode used for reading.
  Serial.print("Resolution in mode: ");
  Serial.println (tempsensor.getResolution());
  float f = tempsensor.readTempF();
  Serial.print("Temp: "); 
  Serial.print(f, 4);
  Serial.println(" F");
  Blynk.virtualWrite(V5, f);

  delay(200);
  Serial.println("Shutdown MCP9808.... ");
  tempsensor.shutdown_wake(1); // shutdown MSP9808 - power consumption ~0.1 mikro Ampere, stops temperature sampling
  digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off
}

void loop() {
  Blynk.run();
  timer.run();
}
