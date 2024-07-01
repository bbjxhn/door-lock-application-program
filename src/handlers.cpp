#include "handlers.h"
#include "tasks.h"
#include <ESPAsyncWebServer.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <ESP32Servo.h>
#include "password_utils.h"

#define SS (5)

bool isAddingNewTag;

extern Servo SERVO;
extern String savedPassword;
extern bool rfidScanSuccess;
extern SemaphoreHandle_t scanSemaphore;

MFRC522 rfid(SS, -1);

AsyncWebServerRequest *pendingRequest = nullptr;

void sendCORSHeaders(AsyncWebServerResponse *response) {
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
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

void handleScanRequest(AsyncWebServerRequest *request) {
    bool success = false;

    if (xSemaphoreTake(scanSemaphore, portMAX_DELAY) == pdTRUE) {
        success = rfidScanSuccess;
        rfidScanSuccess = false;
        xSemaphoreGive(scanSemaphore);
    }

    if (success) {
        String response = "success";
        request->send(200, "text/plain", response);
    } else {
        String response = "failed";
        request->send(200, "text/plain", response);
    }
}

void handleNewTagRequest(AsyncWebServerRequest *request) {
    isAddingNewTag = true;
    pendingRequest = request;  
}

void respondToPendingRequest() {
    if (pendingRequest) {
        String response = "{\"status\":\"Tag added successfully\"}";
        AsyncWebServerResponse *resp = pendingRequest->beginResponse(200, "application/json", response);
        sendCORSHeaders(resp);
        pendingRequest->send(resp);
        pendingRequest = nullptr;
    }
}