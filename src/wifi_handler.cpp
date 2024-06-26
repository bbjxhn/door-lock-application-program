#include "wifi_handler.h" 
#include <WiFi.h>
#include "secrets.h"

void connect_wifi() {
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.print("Connected to ");
    Serial.println(WIFI_SSID);
}

void connect_wifi_enterprise() {
    int counter = 0;
    WiFi.mode(WIFI_STA);

    WiFi.begin(EAP_WIFI_SSID, WPA2_AUTH_PEAP, EAP_USERNAME, EAP_USERNAME, EAP_PASSWORD);

    Serial.print("Connecting to ");
    Serial.println(EAP_WIFI_SSID);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        counter++;

        if (counter >= 60) {
            ESP.restart();
        }
    }
    Serial.print("Connected to ");
    Serial.println(EAP_WIFI_SSID);
}

void connect_hotspot() {
    Serial.print("Connecting to ");
    Serial.println(HOTSPOT_SSID);
    WiFi.begin(HOTSPOT_SSID, HOTSPOT_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.print("Connected to ");
    Serial.println(HOTSPOT_SSID);
}