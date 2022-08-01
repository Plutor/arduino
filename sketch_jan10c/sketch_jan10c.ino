const int pin = 4;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Ready");
  pinMode(pin, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  int val = -1;
  int dur = 0;
  while (int now = digitalRead(15)) {
    yield();
    dur++;
    if (val == now) {
      continue;
    }
    Serial.print("pin ");
    Serial.print(pin);
    Serial.print(" = ");
    Serial.print(now, HEX);
    Serial.print(" after ");
    Serial.print(dur);
    Serial.println("ms");
    val = now;
    dur = 0;
  }
}
