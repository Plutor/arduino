#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <ir_Daikin.h>

const uint16_t kSendPin = 14;

//IRDaikin64 ir(kSendPin);
//IRDaikin128 ir(kSendPin);
//IRDaikin176 ir(kSendPin);
//IRDaikin152 ir(kSendPin);
//IRDaikin160 ir(kSendPin);
//IRDaikin2 ir(kSendPin);
//IRDaikin216 ir(kSendPin);
IRDaikinESP ir(kSendPin);
 
decode_results results;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(kSendPin, OUTPUT);
  Serial.begin(115200);
  ir.begin();
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);
  Serial.println();
  Serial.print("irdaikin ready on pin ");
  Serial.println(kSendPin);
}

void loop() {
  delay(3000);
  digitalWrite(LED_BUILTIN, LOW);
  ir.on();
  ir.setMode(kDaikinHeat);
  ir.setTemp(18); // 18C = 64.6F;
  Serial.println(ir.toString());
  ir.send();
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(3000);
  digitalWrite(LED_BUILTIN, LOW);
  ir.on();
  ir.setMode(kDaikinHeat);
  ir.setTemp(10); // 10C = 50F;
  Serial.println(ir.toString());
  ir.send();
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
}
