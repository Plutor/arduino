#include <IRsend.h>
#include <Arduino.h>

#define PIN_IR     15
#define PIN_DETECT 4

IRsend irsend(PIN_IR);

int lastval = -1;
int lastchange = -1;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("starting");
  pinMode(PIN_DETECT, INPUT);
  pinMode(PIN_IR, OUTPUT);
}

void loop() {
  // Turn IR on
  digitalWrite(PIN_IR, HIGH);
  delay(10);
  // Read
  int val = analogRead(PIN_DETECT);
  // Turn off
  digitalWrite(PIN_IR, LOW);

  if (lastval != val) {
    unsigned long n = millis();
    if (lastchange == -1) { lastchange = n; }
    char buf[100];
    sprintf(buf, "Val is now %d, first change in %ldms", val, n-lastchange);
    Serial.println(buf);
    lastval = val;
    lastchange = n;
  }
}
