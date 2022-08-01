#include <BlynkSimpleEsp8266.h>

#include <IRsend.h>
#include <Arduino.h>

#define PIN_IR      15
#define PIN_DETECT  4
#define BLYNK_AUTH  "XXXXXXXX"
#define WIFI_SSD    "XXXXXXXX"
#define WIFI_PASSWD "XXXXXXXX"
#define BLYNK_DEBUG // Optional, this enables lots of prints
#define BLYNK_PRINT Serial

IRsend irsend(PIN_IR);
int lastval = -1;
int lastchange;
int bubbleCount = 0;
BlynkTimer timer;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("starting bubblecounter");
  pinMode(PIN_DETECT, INPUT);
  pinMode(PIN_IR, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Init Blynk
  Blynk.begin(BLYNK_AUTH, WIFI_SSD, WIFI_PASSWD);
  // Once a minute upload data to Blynk.
  timer.setInterval(60000L, reportBubbleCount);

  Serial.println("ready");
  lastchange = millis();
}

void loop() {
  Blynk.run();
  timer.run();
  // Turn IR on
  digitalWrite(PIN_IR, HIGH);
  delay(10);
  // Read
  int val = analogRead(PIN_DETECT);
  // Turn off
  digitalWrite(PIN_IR, LOW);

  // On if val is low, which means we're getting a signal.
  digitalWrite(LED_BUILTIN, (val < 100) ? LOW : HIGH);

  if (lastval != val) {
    unsigned long n = millis();
    if (lastchange == -1) { lastchange = n; }

    // If this is a transition more than 10% of the sensor's range, log it.
    if (abs(lastval - val) > 100) {
      char buf[100];
      sprintf(buf, "Val is now %d, first change in %ldms", val, n-lastchange);
      Serial.println(buf);
      lastchange = n;

      // If higher -> more blocked -> transition from air to water.
      if (val > lastval) {
        bubbleCount++;
      }
    }
    lastval = val;
  }
}

void reportBubbleCount() {
  int c = bubbleCount;
  bubbleCount = 0;
  Serial.print("Reporting bubbleCount=");
  Serial.print(c);  
  Serial.print(", latest val=");
  Serial.println(lastval);  
  Blynk.virtualWrite(V0, c);
  Blynk.virtualWrite(V1, lastval);
}
