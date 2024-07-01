#include "password_utils.h"
#include <SPIFFS.h>

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

    password = file.readString();
    file.close();
    
    return true;
}