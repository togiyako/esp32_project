#include "Data_Logger.h"
#include "SD_Card.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static char buffer1[BUFFER_LINES][LINE_SIZE];
static char buffer2[BUFFER_LINES][LINE_SIZE];

static volatile char (*volatileWriteBuffer)[LINE_SIZE] = buffer1;
static volatile char (*volatileReadBuffer)[LINE_SIZE] = buffer2;

static int writeIndex = 0;
static volatile bool bufferReady = false;

static SemaphoreHandle_t bufferSemaphore;

static void BufferSwitcherTask(void *pvParameters);
static void FileWriterTask(void *pvParameters);

void DataLogger_Init() {
  bufferSemaphore = xSemaphoreCreateBinary();
  if (bufferSemaphore == NULL) {
    Serial.println("❌ Failed to create semaphore");
    return;
  }

  xTaskCreatePinnedToCore(BufferSwitcherTask, "BufferSwitcher", 2048, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(FileWriterTask, "FileWriter", 4096, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
}

void DataLogger_Loop() {
  static String inputLine = "";

  while (Serial.available()) {
    char ch = Serial.read();

    if (ch == '\n') {
      inputLine.trim();

      // Замінюємо пробіли та табуляції на коми
      inputLine.replace('\t', ',');
      inputLine.replace(' ', ',');

      if (writeIndex < BUFFER_LINES) {
        inputLine.toCharArray((char*)volatileWriteBuffer[writeIndex], LINE_SIZE);
        writeIndex++;
      }
      inputLine = "";
    } else {
      inputLine += ch;
    }
  }
}

static void BufferSwitcherTask(void *pvParameters) {
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(19);

  for (;;) {
    vTaskDelayUntil(&lastWakeTime, period);

    if (!bufferReady && writeIndex > 0) {
      char (*temp)[LINE_SIZE] = (char (*)[LINE_SIZE])volatileWriteBuffer;
      volatileWriteBuffer = volatileReadBuffer;
      volatileReadBuffer = temp;
      writeIndex = 0;
      bufferReady = true;
      xSemaphoreGive(bufferSemaphore);
    }
  }
}

static void FileWriterTask(void *pvParameters) {
  for (;;) {
    if (xSemaphoreTake(bufferSemaphore, portMAX_DELAY) == pdTRUE) {
      Serial.println("📝 Writing buffer to SD...");

      for (int i = 0; i < BUFFER_LINES; i++) {
        char *line = (char *)volatileReadBuffer[i];
        if (strlen(line) > 0) {
          Log_Write(line);
          Log_Write("\n");
        }
      }

      Log_FlushToSD();

      for (int i = 0; i < BUFFER_LINES; i++) {
        volatileReadBuffer[i][0] = '\0';
      }

      bufferReady = false;
    }
  }
}
