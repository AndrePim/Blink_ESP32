#include <Arduino.h>

void setup() {
  pinMode(13, OUTPUT);
}

void loop() {
  digitalWrite(13, ! digitalRead(13));
  delay(500);
  // put your main code here, to run repeatedly:
}