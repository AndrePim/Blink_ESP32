// Урок:
// https://www.youtube.com/watch?v=KosUkJmHWxI
#include <Arduino.h>

enum ledmode_t : uint8_t
{
  LED_OFF,
  LED_ON,
  LED_1HZ, // 1 Гц
  LED_2HZ, // 2 Гц
  LED_4HZ  // 4 Гц
};

const uint8_t LED_PIN = 13; // Светодиод 13 пин
const bool LED_LEVEL = HIGH;

TaskHandle_t blink;

void blinkTask(void *pvParam)
{
  const uint32_t LED_PULSE = 25; // 25 ms.

  ledmode_t ledmode = LED_OFF;

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, !LED_LEVEL);
  while (true)
  {
    uint32_t notifyValue;

    if (xTaskNotifyWait(0, 0, &notifyValue, ledmode < LED_1HZ ? portMAX_DELAY : 0) == pdTRUE)
    {
      ledmode = (ledmode_t)notifyValue;
      if (ledmode == LED_OFF)
        digitalWrite(LED_PIN, !LED_LEVEL);
      else if (ledmode == LED_ON)
        digitalWrite(LED_PIN, LED_LEVEL);
    }
    if (ledmode >= LED_1HZ)
    {
      digitalWrite(LED_PIN, LED_LEVEL);
      vTaskDelay(pdMS_TO_TICKS(LED_PULSE));
      digitalWrite(LED_PIN, !LED_LEVEL);
      vTaskDelay(pdMS_TO_TICKS((ledmode == LED_1HZ ? 1000 : ledmode == LED_2HZ ? 500
                                                                               : 250) - LED_PULSE));
    }
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  if (xTaskCreate(blinkTask, "blink", 1024, NULL, 1, &blink) != pdPASS)
  {
    Serial.println("Error creating blink task!");
    Serial.flush();
    esp_deep_sleep_start();
  }
}

void loop()
{
  static ledmode_t ledmode = LED_OFF;
  if (ledmode < LED_4HZ)
    ledmode = (ledmode_t)((uint8_t)ledmode + 1);
  else
    ledmode = LED_OFF;
  if (xTaskNotify(blink, ledmode, eSetValueWithOverwrite) != pdPASS)
  {
    Serial.println("Error setting LED mode!");
  }
  vTaskDelay(pdMS_TO_TICKS(5000));
}