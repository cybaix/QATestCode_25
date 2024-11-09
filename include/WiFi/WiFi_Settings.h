#ifndef WIFI_SETTINGS_H
#define WIFI_SETTINGS_H

#include <WiFi.h>
#include "../LED/Status_LED.h"  // Updated path to Status_LED.h
#include "../Version.h"  // Include version information

// Define a struct to hold Wi-Fi credentials
struct WiFiNetwork {
    const char* ssid;
    const char* password;
};

// List of Wi-Fi networks (add more networks as needed)
WiFiNetwork wifiNetworks[] = {
    {"BenNet", "River11111"},  // Updated SSID3 and password
    {"SSID_1", "Password_1"},
    {"SSID_2", "Password_2"}
    
};

// Number of networks in the list
const int numNetworks = sizeof(wifiNetworks) / sizeof(wifiNetworks[0]);

// Function to fade out the green LED after Wi-Fi connection
void fadeOutGreen() {
    for (int brightness = 255; brightness >= 0; brightness -= 5) {
        statusLED.setPixelColor(0, statusLED.Color(0, brightness, 0)); // Gradual green fade
        statusLED.show();
        delay(200); // Delay to achieve a smooth fade over 10 seconds
    }
    clearLED();  // Turn off the LED after fading
}

// Function to connect to Wi-Fi with persistence enabled
void connectToWiFi() {
    Serial.println("Setting up Wi-Fi with persistence...");

    // Enable Wi-Fi persistence and auto-reconnect features
    WiFi.persistent(true);       // Store credentials in non-volatile storage
    WiFi.setAutoReconnect(true);  // Automatically reconnect if disconnected

    Serial.print("Starting Wi-Fi connection (Code Version: ");
    Serial.print(CODE_VERSION);
    Serial.println(")");

    // Set LED to solid yellow during the connection attempt
    setSolidYellow();

    // Loop through each network and attempt to connect
    for (int i = 0; i < numNetworks; i++) {
        Serial.print("Trying to connect to ");
        Serial.println(wifiNetworks[i].ssid);

        WiFi.begin(wifiNetworks[i].ssid, wifiNetworks[i].password);

        // Attempt to connect for up to 10 seconds
        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
            delay(500);
            Serial.print(".");
        }

        // Check if connected
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConnected to Wi-Fi!");
            Serial.print("Connected to: ");
            Serial.println(wifiNetworks[i].ssid);
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            setSolidGreen();  // Set LED to solid green when connected
            fadeOutGreen();   // Gradually fade out the green LED after connection
            return;           // Exit the function after a successful connection
        } else {
            Serial.println("\nFailed to connect to Wi-Fi.");
        }
    }

    // If all networks fail, set LED to red
    setSolidRed();
    Serial.println("Unable to connect to any configured networks.");
}

#endif // WIFI_SETTINGS_H
