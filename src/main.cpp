#include "handlers.h"
#include "tasks.h"
#include "wifi_handler.h"
#include "password_utils.h"

#include <HTTPClient.h>
#include <freertos/semphr.h>
#include <AsyncTCP.h>

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
const char* serverStoreName = "http://172.20.10.3:3000/api/scanned-uid";

float batteryVoltage = 0.0;

bool isAuthActive = false;
SemaphoreHandle_t xSemaphore;

void sendUIDToServer(String uid) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverStoreName);
        http.addHeader("Content-Type", "application/json");
        String jsonPayload = "{\"uid\":\"" + uid + "\"}";
        int httpResponseCode = http.POST(jsonPayload);

        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println("HTTP Response code: " + String(httpResponseCode));
            Serial.println("Server response: " + response);
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

    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    SERVO.attach(SERVO_PIN);

    keypad.begin();

    server.on("/lock", HTTP_GET, handleLock);
    server.on("/password-generator", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, handlePasswordGenerator);
    server.on("/scan", HTTP_GET, handleScanRequest);
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
