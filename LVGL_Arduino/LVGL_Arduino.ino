#include "SD_Card.h"
#include "Display_ST7789.h"
#include "LVGL_Driver.h"
#include "LVGL_Example.h"
#include "Data_Logger.h"


void setup() {
  Serial.begin(38400);
  delay(1000);

  LCD_Init();
  Lvgl_Init();
  SD_Init();
  DataLogger_Init();

  Lvgl_Example1();
  //Flash_test();
  // lv_demo_widgets();               
  // lv_demo_benchmark();          
  // lv_demo_keypad_encoder();     
  // lv_demo_music();  
  // lv_demo_stress();   
  //Wireless_Test2(); 
  
}

void loop() {
  DataLogger_Loop();
  Log_CheckInactivity();
}
