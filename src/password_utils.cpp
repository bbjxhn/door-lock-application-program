#include "password_utils.h"
#include <SPIFFS.h>

void setupPassword(String& password) {
    String tempPassword;
    bool isConfirmingPassword = false;

    while (true) {
        if (!isConfirmingPassword) {
            Serial.println("Enter a new 6-digit password:");
            tempPassword = input;
            if (tempPassword.length() != 6) {
                Serial.println("Password must be 6 digits long. Please try again.");
                continue;
            }
            isConfirmingPassword = true;
            Serial.println("Re-enter the password to confirm:");
        } else {
            String confirmPassword = input;
            if (confirmPassword == tempPassword) {
                password = tempPassword;
                Serial.println("Password set successfully.");
                break;
            } else {
                Serial.println("Passwords do not match. Please try again.");
                isConfirmingPassword = false;
            }
        }
    }
}

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

    if (file.size() == 0) {
        Serial.println("Password file is empty. Please set up a new password.");
        setupPassword(password);
        writePasswordToFile(password);
        file.close();
        return true;
    }

    password = file.readString();
    file.close();
    return true;
}