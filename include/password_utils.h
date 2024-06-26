#ifndef PASSWORD_UTILS_H
#define PASSWORD_UTILS_H

#include <Arduino.h>

extern String input;

void setupPassword(String& password);
void writePasswordToFile(const String& password);
bool readPasswordFromFile(String& password);

#endif
