#include <Arduino.h>


const uint8_t LED_PIN = LED_BUILTIN;
const bool LED_LEVEL = HIGH;


void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, | LED_LEVEL);
  // put your setup code here, to run once:
}

void loop() {
  digtalWrite(LED_PIN, LED_LEVEL);
  delay(25);
  digitalWrite(LED_PIN, | LED_LEVEL);
  delay(1000 - 25);
  // put your main code here, to run repeatedly:
}