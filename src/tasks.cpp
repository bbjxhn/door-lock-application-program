#include "tasks.h"
#include "password_utils.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern MFRC522 rfid;
extern void sendUIDToServer(const char* serverEndpoint, String uid, bool isCheck);
extern bool rfidScanSuccess;
extern SemaphoreHandle_t scanSemaphore;
extern bool isAddingNewTag;
extern void respondToPendingRequest();
extern const char* serverCheckUID;
extern const char* serverStoreUID;
extern String currentUID;  // Declare currentUID to store the latest scanned UID

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
                    Serial.println("\nAuthentication successful");
                    isAuthActive = false;
                    input.clear();
                } else {
                    Serial.println("\nAuthentication failed");
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
            sendUIDToServer(serverCheckUID, uid, true);

            if (isAddingNewTag) {
                currentUID = uid;  // Store the current UID
                sendUIDToServer(serverStoreUID, uid, false);

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
