#include <FS.h>
#include <MFRC522.h>
#include <SPI.h>
#include <ESP32Servo.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <Adafruit_Keypad.h>
#include "wifi_handler.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

AsyncWebServer server(80);

#define SCK (18)
#define MOSI (23)
#define MISO (19)
#define SDA (21)

#define KEYPAD_PID1824
#define R4 (32)
#define R3 (33)
#define R2 (13)
#define R1 (26)
#define C3 (27)
#define C2 (14)
#define C1 (12)

#define SERVO_PIN (2)

#include "keypad_config.h"
Adafruit_Keypad keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
MFRC522 mfrc522(SDA, -1);
Servo SERVO;

String input = "";
String savedPassword = "";
String tempPassword = "";

byte authorizedUIDs[][4] = {
    {0x2A, 0x9E, 0x3D, 0x86}
};

const int numberOfAuthorizedUIDs = sizeof(authorizedUIDs) / sizeof(authorizedUIDs[0]);

bool isAuthActive = false;
SemaphoreHandle_t xSemaphore;

void setupPassword(String& password) {
    String tempPassword;
    bool isConfirmingPassword = false;

    while (true) {
        if (!isConfirmingPassword) {
            Serial.println("Enter a new 6-digit password:");
            tempPassword = input;
            if (tempPassword.length() != 6) {
                Serial.println("Password must be 6 digits long. Please try again.");
                continue;
            }
            isConfirmingPassword = true;
            Serial.println("Re-enter the password to confirm:");
        } else {
            String confirmPassword = input;
            if (confirmPassword == tempPassword) {
                password = tempPassword;
                Serial.println("Password set successfully.");
                break;
            } else {
                Serial.println("Passwords do not match. Please try again.");
                isConfirmingPassword = false;
            }
        }
    }
}

void writePasswordToFile(const String& password) {
    if (!SPIFFS.begin(true)) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    File file = SPIFFS.open("/password.txt", FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

    file.close();

    file = SPIFFS.open("/password.txt", FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return;
    }

    if (file.print(password)) {
        Serial.println("Password saved to file");
    } else {
        Serial.println("Failed to write password to file");
    }

    file.close();
}

bool readPasswordFromFile(String& password) {
    if (!SPIFFS.begin(true)) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return false;
    }

    File file = SPIFFS.open("/password.txt", FILE_READ);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return false;
    }

    if (file.size() == 0) {
        Serial.println("Password file is empty. Please set up a new password.");
        setupPassword(password);
        writePasswordToFile(password);
        file.close();
        return true;
    }

    password = file.readString();
    file.close();
    return true;
}

void handleLock(AsyncWebServerRequest *request) {
    String state = request->getParam("state")->value();
    if (state == "unlocked") {
        SERVO.write(180);
        request->send(200, "text/plain", "Unlocked");
    } else if (state == "locked") {
        SERVO.write(-180);
        request->send(200, "text/plain", "Locked");
    } else {
        request->send(400, "text/plain", "Invalid state");
    }
}

void handlePasswordGenerator(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String newPassword = "";
    for (size_t i = 0; i < len; i++) {
        char c = static_cast<char>(data[i]);
        if (isdigit(c)) {
            newPassword += c;
        }
    }

    if (newPassword.length() == 6) {
        Serial.print("New password received from web server: ");
        Serial.println(newPassword);

        savedPassword = newPassword;
        writePasswordToFile(savedPassword);

        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "success");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    } else {
        Serial.println("Invalid password length");
        AsyncWebServerResponse *response = request->beginResponse(400, "text/plain", "Invalid password. Password must be 6 digits long.");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    }
}

bool isAuthorized(byte *uid, byte size) {
    for (byte i = 0; i < sizeof(authorizedUIDs) / sizeof(authorizedUIDs[0]); i++) {
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

void setup() {
    Serial.begin(115200);
    connect_hotspot();

    SPI.begin(SCK, MISO, MOSI); 
    mfrc522.PCD_Init();

    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    SERVO.attach(SERVO_PIN);

    keypad.begin();
    
    server.on("/lock", HTTP_GET, handleLock);
    server.on("/password-generator", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, handlePasswordGenerator);
    server.begin();

    Serial.println("\nSetup complete");   

    xSemaphore = xSemaphoreCreateMutex();
    if (xSemaphore == NULL) {
        Serial.println("Unable to create semaphore");
        while (1);
    }

    xTaskCreate(rfidTask, "rfidTask", 4096, NULL, 1, NULL);
    xTaskCreate(keypadTask, "keypadTask", 4096, NULL, 1, NULL);
}

void loop() {}
