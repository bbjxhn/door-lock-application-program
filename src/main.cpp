#include "handlers.h"
#include "tasks.h"
#include "wifi_handler.h"
#include "password_utils.h"

#include <HTTPClient.h>
#include <freertos/semphr.h>
#include <vector>

HTTPClient http;
AsyncWebServer server(80);

#define SCK (18)
#define SS (5)
#define MOSI (23)
#define MISO (19)

#define KEYPAD_PID1824
#define R4 (32)
#define R3 (33)
#define R2 (13)
#define R1 (26)
#define C3 (27)
#define C2 (14)
#define C1 (12)

#include "keypad_config.h"
Adafruit_Keypad keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
MFRC522 mfrc522(SS, -1);

String input = "";
String savedPassword = "";
const char* serverCheckUID = "http://172.20.10.3:3000/api/check-uid";
const char* serverStoreUID = "http://172.20.10.3:3000/api/store-uid";

bool isAuthActive = false;
SemaphoreHandle_t xSemaphore;

bool rfidScanSuccess = false;
SemaphoreHandle_t scanSemaphore;

String currentUID;  

void sendUIDToServer(const char* serverEndpoint, String uid, bool isCheck) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverEndpoint);
        http.addHeader("Content-Type", "application/json");
        String jsonPayload = "{\"uid\":\"" + uid + "\"}";
        int httpResponseCode = http.POST(jsonPayload);

        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println("HTTP Response code: " + String(httpResponseCode));
            Serial.println("Server response: " + response);

            if (isCheck) {
                if (response == "UID Matched!") {
                    digitalWrite(2, HIGH);  // Turn on the LED
                } else {
                    digitalWrite(2, LOW);   // Turn off the LED
                }
            } else {
                if (httpResponseCode == 409) {
                    Serial.println("Duplicate UID detected, not turning on the lock.");
                } else if (response == "UID stored successfully") {
                    Serial.println("New UID stored successfully.");
                } else {
                    Serial.println("Failed to store UID.");
                }
            }
        } else {
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);
        }
        http.end();
    } else {
        Serial.println("WiFi not connected");
    }
}

void setup() {
    Serial.begin(115200);
    connect_hotspot();

    SPI.begin(SCK, MISO, MOSI); 
    mfrc522.PCD_Init();

    pinMode(2, OUTPUT);

    keypad.begin();

    server.on("/lock", HTTP_GET, handleLock);
    server.on("/password-generator", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, handlePasswordGenerator);
    server.on("/scan", HTTP_GET, handleScanRequest);
    server.on("/new-tag", HTTP_GET, handleNewTagRequest);
    server.on("/new-tag", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(204);
        sendCORSHeaders(response);
        request->send(response);
    });
    server.begin();

    Serial.println("\nSetup complete");   

    xSemaphore = xSemaphoreCreateMutex();
    if (xSemaphore == NULL) {
        Serial.println("Unable to create semaphore");
        while (1);
    }

    scanSemaphore = xSemaphoreCreateMutex();
    if (scanSemaphore == NULL) {
        Serial.println("Unable to create semaphore");
        while (1);
    }

    esp_task_wdt_init(10, true);
    esp_task_wdt_add(NULL);

    xTaskCreate(rfidTask, "rfidTask", 4096, NULL, 1, NULL);
    xTaskCreate(keypadTask, "keypadTask", 4096, NULL, 1, NULL);
}

void loop() {
    esp_task_wdt_reset();
}
