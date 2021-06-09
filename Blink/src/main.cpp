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
const uint8_t BTN_PIN = 2;   // Кнопка 0 пин
const bool BTN_LEVEL = LOW;  // Высокий уровень сигнала

TaskHandle_t blink;
QueueHandle_t queue;
SemaphoreHandle_t semaphore;
portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;

uint8_t lastPressed = 0;
bool lastBtn;

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

void IRAM_ATTR btnISR()
{
  portENTER_CRITICAL_ISR(&mutex);
  lastBtn = digitalRead(BTN_PIN) == BTN_LEVEL;
  if (lastBtn)
    lastPressed = millis();
  portEXIT_CRITICAL_ISR(&mutex);
  xSemaphoreGiveFromISR(semaphore, NULL);
}

void btnTask(void *pvParam)
{
  // Избежание дребезга контактов на кнопке
  const uint32_t CLICK_TIME = 20;      // 20 ms.
  const uint32_t LONGCLICK_TIME = 500; // 500 ms.

  pinMode(BTN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN_PIN), btnISR, CHANGE);

  while (true)
  {
    if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE)
    {
      uint32_t time = millis();
      buttonstate_t state;
      if (lastBtn)
      {
        state = BTN_PRESSED;
        lastPressed = time;
      }
      else
      {
        if (time - lastPressed >= LONGCLICK_TIME)
          state = BTN_LONGCLICK;
        else if (time - lastPressed >= CLICK_TIME)
          state = BTN_CLICK;
        else
          state = BTN_RELEASED;
        lastPressed = 0;
      }
      xQueueSend((QueueHandle_t)pvParam, &state, portMAX_DELAY);
    }
  }
}
// Функция для очистки буфера и отправки esp в глубокий сон
static void halt(const char *msg)
{
  Serial.println(msg);
  Serial.flush();
  esp_deep_sleep_start();
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  if (xTaskCreate(blinkTask, "blink", 1024, NULL, 1, &blink) != pdPASS)
    halt("Error creating blink task!");

  queue = xQueueCreate(32, sizeof(buttonstate_t)); // Очередь
  if (!queue)
    halt("Error creating queue!");

  semaphore = xSemaphoreCreateBinary();
  if (!semaphore)
    halt("Error creating semaphore!");

  if (xTaskCreate(btnTask, "btn", 1024, (void *)queue, 1, NULL) != pdPASS)
    halt("Error creating button task!");

  if (xTaskNotify(blink, LED_1HZ, eSetValueWithOverwrite) != pdPASS)
    Serial.println("Error setting LED mode!");
}

void loop()
{
  buttonstate_t state;

  if (xQueueReceive(queue, &state, portMAX_DELAY) == pdTRUE)
  {
    Serial.print("Button ");
    switch (state)
    {
    case BTN_RELEASED:
      Serial.println("released");
      break;
    case BTN_PRESSED:
      Serial.println("pressed");
      break;
    case BTN_CLICK:
      Serial.println("clicked");
      break;
    case BTN_LONGCLICK:
      Serial.println("long clicked");
      break;
    }
  }
}