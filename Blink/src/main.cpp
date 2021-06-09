// Урок:
// https://www.youtube.com/watch?v=KosUkJmHWxI
#include <Arduino.h>

// Состояния для светодиода
enum ledmode_t : uint8_t
{
  LED_OFF,
  LED_ON,
  LED_1HZ, // 1 Гц
  LED_2HZ, // 2 Гц
  LED_4HZ  // 4 Гц
};

// Состояния для кнопки
enum buttonstate_t : uint8_t
{
  BTN_RELEASED,
  BTN_PRESSED,
  BTN_CLICK,
  BTN_LONGCLICK
};

const uint8_t LED_PIN = 13;  // Светодиод 13 пин
const bool LED_LEVEL = HIGH; // Высокий уровень сигнала
const uint8_t BTN_PIN = 0;   // Кнопка 0 пин
const bool BTN_LEVEL = HIGH; // Высокий уровень сигнала

TaskHandle_t blink;
QueueHandle_t queue;

// Задача FreeRTOS для мигания светодиода
void blinkTask(void *pvParam)
{
  const uint32_t LED_PULSE = 25; // 25 ms.

  ledmode_t ledmode = LED_OFF;

  pinMode(LED_PIN, OUTPUT);          // Режим работы пина LED_PIN
  digitalWrite(LED_PIN, !LED_LEVEL); // Выключение светодиода
  while (true)
  {
    uint32_t notifyValue;

    if (xTaskNotifyWait(0, 0, &notifyValue, ledmode < LED_1HZ ? portMAX_DELAY : 0) == pdTRUE)
    {
      ledmode = (ledmode_t)notifyValue;
      if (ledmode == LED_OFF) // Выключение светодиода
        digitalWrite(LED_PIN, !LED_LEVEL);
      else if (ledmode == LED_ON) // Включение светодиода
        digitalWrite(LED_PIN, LED_LEVEL);
    }
    if (ledmode >= LED_1HZ)
    {
      digitalWrite(LED_PIN, LED_LEVEL);
      vTaskDelay(pdMS_TO_TICKS(LED_PULSE));
      digitalWrite(LED_PIN, !LED_LEVEL);
      vTaskDelay(pdMS_TO_TICKS((ledmode == LED_1HZ ? 1000 : ledmode == LED_2HZ ? 500
                                                                               : 250) -
                               LED_PULSE));
    }
  }
}

void btnTask(void *pvParam)
{
  // Избежание дребезга контактов на кнопке
  const uint32_t CLICK_TIME = 20; // 20 ms.
  const uint32_t LONGCLICK_TIME = 500; // 500 ms. 

  uint8_t lastPressed = 0;
  bool lastBtn;

  pinMode(BTN_PIN, INPUT_PULLUP);
  lastBtn = digitalRead(BTN_PIN) == BTN_LEVEL;

  while (true)
  {
    bool btn = digitalRead(BTN_PIN) == BTN_LEVEL;
    if (btn != lastBtn)
    {
      uint32_t time = millis();
      buttonstate_t state;
      if (btn)
      {
        state = BTN_PRESSED;
        lastPressed = time;
      } else {
        if(time - lastPressed >= LONGCLICK_TIME) 
          state = BTN_LONGCLICK;
        else if (time - lastPressed >= CLICK_TIME)
          state = BTN_CLICK;
        else 
          state = BTN_RELEASED;
        lastPressed = 0;
      }
      xQueueSend((QueueHandle_t)pvParam, &state, portMAX_DELAY);
      lastBtn = btn;
    }
  }
}

// Функция для очистки буфера и отправли
static void halt(const char *msg){
    Serial.println(msg);
    Serial.flush();
    esp_deep_sleep_start();
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  if (xTaskCreate(blinkTask, "blink", 1024, NULL, 1, &blink) != pdPASS)
  {
    halt("Error creating blink task!");
  }
  if (queue = xQueueCreate(32, sizeof(buttonstate_t))); // Очередь
  if (! queue){
    halt("Error creating queue!");
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
// https://www.youtube.com/watch?v=KosUkJmHWxI&t=3s