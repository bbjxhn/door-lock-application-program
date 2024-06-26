#include "handlers.h"
#include <ESPAsyncWebServer.h>
#include <Arduino.h>
#include <ESP32Servo.h>
#include "password_utils.h"

extern Servo SERVO;
extern String savedPassword;

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
