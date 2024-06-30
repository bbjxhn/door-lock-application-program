#include "tasks.h"
#include "password_utils.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern MFRC522 rfid;
extern void sendUIDToServer(String uid);
extern bool rfidScanSuccess;
extern SemaphoreHandle_t scanSemaphore;
extern bool isAddingNewTag;
extern void respondToPendingRequest();

void keypadTask(void *param) {
    if (!readPasswordFromFile(savedPassword)) {
        Serial.println("Failed to read password from file.");
        vTaskDelete(NULL);
        return;
    }

    int attempts = 0;
    while (true) {
        esp_task_wdt_reset();

        keypad.tick();
        keypadEvent event = keypad.read();
        if (event.bit.EVENT == KEY_JUST_PRESSED) {
            char key = (char)event.bit.KEY;
            if (key == '*') {
                if (input == savedPassword) {
                    SERVO.write(180);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    SERVO.write(-180);
                    isAuthActive = false;
                    input.clear();
                } else {
                    input.clear();
                }
            } else if (key == '#' && input.length() > 0) {
                input.remove(input.length() - 1);
            } else {
                input += key;
            }
            Serial.print("\r");
            Serial.print("                         ");
            Serial.print("\r");
            Serial.print(input);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void rfidTask(void *param) {
    while (true) {
        esp_task_wdt_reset();

        if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
            String uid = "";
            for (byte i = 0; i < rfid.uid.size; i++) {
                uid += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
                uid += String(rfid.uid.uidByte[i], HEX);
            }
            uid.toUpperCase();

            if (isAddingNewTag) {
                sendUIDToServer(uid);

                if (xSemaphoreTake(scanSemaphore, portMAX_DELAY) == pdTRUE) {
                    rfidScanSuccess = true;
                    xSemaphoreGive(scanSemaphore);
                }
                isAddingNewTag = false; 
                respondToPendingRequest(); 
            }
            rfid.PICC_HaltA();
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); 
    }
}
