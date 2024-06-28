#include "handlers.h"
#include "tasks.h"
#include <ESPAsyncWebServer.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <ESP32Servo.h>
#include "password_utils.h"

#define SDA (21)

extern Servo SERVO;
extern String savedPassword;

MFRC522 rfid(SDA, -1);
extern void sendUIDToServer(String uid);

#define BATTERY_PIN (25)

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

void handleScanRequest(AsyncWebServerRequest *request) {
    while (true) {
        if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
            String uid = "";
            for (byte i = 0; i < rfid.uid.size; i++) {
                uid += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
                uid += String(rfid.uid.uidByte[i], HEX);
            }
            uid.toUpperCase();
            sendUIDToServer(uid);
            request->send(200, "application/json", "{\"success\": true, \"uid\": \"" + uid + "\"}");
            rfid.PICC_HaltA();
            break;
        } else {
            request->send(500, "application/json", "{\"success\": false}");
        }
    }
}