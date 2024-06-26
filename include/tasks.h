#ifndef TASKS_H
#define TASKS_H

#include <Arduino.h>
#include <Adafruit_Keypad.h>
#include <MFRC522.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>

extern Adafruit_Keypad keypad;
extern MFRC522 mfrc522;
extern String input;
extern String savedPassword;
extern bool isAuthActive;
extern SemaphoreHandle_t xSemaphore;

void keypadTask(void *param);
void rfidTask(void *param);

#endif
