#include "SD_Card.h"
#include "Display_ST7789.h"
#include "LVGL_Driver.h"
#include "LVGL_Example.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "soc/rtc.h"

TaskHandle_t loopTaskHandle1 = NULL;
SemaphoreHandle_t dataReadySemaphore;

const int MAX_TOKENS = 5;
int numbers[MAX_TOKENS];  
String inputString = "";   
bool stringComplete = false; 

void loopTask1(void *pvParameters) {
  for (;;) {
    if (xSemaphoreTake(dataReadySemaphore, portMAX_DELAY) == pdTRUE) {
      Serial.println("🟢 Data processing and writing to a file");
      
      for (int i = 0; i < MAX_TOKENS; i++) {
        if (i > 0) {
          Log_Write(",");  
        }
        Log_Write(String(numbers[i]).c_str());  
      }
      Log_Write("\n");  
      
      Log_FlushToSD();
      Serial.println("🟡 Recording is complete");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("🟢 Main setup: start");

  LCD_Init();
  Lvgl_Init();
  //Lvgl_Example1();  
  SD_Init();

  //Flash_test();
  // lv_demo_widgets();               
  // lv_demo_benchmark();          
  // lv_demo_keypad_encoder();     
  // lv_demo_music();  
  // lv_demo_stress();   
  //Wireless_Test2();  
  
  dataReadySemaphore = xSemaphoreCreateBinary();
  if (dataReadySemaphore == NULL) {
    Serial.println("Error creating a semaphore");
    return;
  }

  xTaskCreateUniversal(loopTask1, "loopTask1", getArduinoLoopTaskStackSize(), NULL, 1, &loopTaskHandle1, ARDUINO_RUNNING_CORE);

  Serial.println("🟢 Main setup: finish");
}

void loop() {
  
  if (Serial.available()) {
    static char inChar;
    static bool dataReady = false;
    
   
    while (Serial.available()) {
      inChar = (char)Serial.read();
      
      if (inChar == '\n') {  
        stringComplete = true;
        break;
      } else {
        inputString += inChar;
      }
    }

    // If the input is complete, process and trigger the record
    if (stringComplete) {
      inputString.trim();
      
      int tokenIndex = 0;
      int fromIndex = 0;
      
      while (fromIndex < inputString.length() && tokenIndex < MAX_TOKENS) {
        int commaIndex = inputString.indexOf(',', fromIndex);
        String token;

        if (commaIndex == -1) {
          token = inputString.substring(fromIndex);
          fromIndex = inputString.length(); 
        } else {
          token = inputString.substring(fromIndex, commaIndex);
          fromIndex = commaIndex + 1;
        }

        numbers[tokenIndex] = token.toInt();  
        Serial.print("Number ");
        Serial.print(tokenIndex);
        Serial.print(": ");
        Serial.println(numbers[tokenIndex]);
        tokenIndex++;
      }

      
      inputString = "";
      stringComplete = false;

      
      xSemaphoreGive(dataReadySemaphore);
    }
  } 
}
