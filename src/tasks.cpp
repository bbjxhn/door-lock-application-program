#include "tasks.h"
#include "password_utils.h"
#include <Arduino.h>

bool isAuthorized(byte *uid, byte size) {
    for (byte i = 0; i < numberOfAuthorizedUIDs; i++) {
        if (memcmp(uid, authorizedUIDs[i], size) == 0) {
            return true;
        }
    }
    return false;
}

void keypadTask(void *param) {
    if (!readPasswordFromFile(savedPassword)) {
        Serial.println("Failed to read password from file.");
        vTaskDelete(NULL);
        return;
    }

    int attempts = 0;
    while (true) {
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
        if (!mfrc522.PICC_IsNewCardPresent()) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (!mfrc522.PICC_ReadCardSerial()) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (isAuthorized(mfrc522.uid.uidByte, mfrc522.uid.size)) {
            isAuthActive = true;
            SERVO.write(180);
            vTaskDelay(pdMS_TO_TICKS(1000));
            SERVO.write(-180);
        } else {
            isAuthActive = false;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
