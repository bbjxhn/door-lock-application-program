#ifndef HANDLERS_H
#define HANDLERS_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

void handleLock(AsyncWebServerRequest *request);
void handlePasswordGenerator(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);

#endif
