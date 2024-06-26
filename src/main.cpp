#include <FS.h>
#include <MFRC522.h>
#include <SPI.h>
#include <ESP32Servo.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <Adafruit_Keypad.h>

#include "handlers.h"
#include "tasks.h"
#include "wifi_handler.h"
#include "password_utils.h"

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

byte authorizedUIDs[][4] = {
    {0x2A, 0x9E, 0x3D, 0x86}
};

const int numberOfAuthorizedUIDs = sizeof(authorizedUIDs) / sizeof(authorizedUIDs[0]);

bool isAuthActive = false;
SemaphoreHandle_t xSemaphore;

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
