#include <WiFi.h>
#include "WiFi/WiFi_Module.h"
#include "LED/Status_LED.h"  // Include to control status LED

// Function to scan Wi-Fi networks and update the status LED
void scanWiFiNetworks() {
    Serial.println("Starting Wi-Fi scan...");

    // Set LED to solid yellow during scanning
    setSolidYellow();

    // Disconnect and reset Wi-Fi settings
    WiFi.disconnect(true);  // Clears saved credentials
    WiFi.mode(WIFI_STA);
    delay(100);

    int numNetworks = WiFi.scanNetworks();

    if (numNetworks == -1) {
        Serial.println("No Wi-Fi networks found or unable to start scan.");
        setSolidRed();  // Solid red if no networks found
        return;
    }

    Serial.print("Found ");
    Serial.print(numNetworks);
    Serial.println(" Wi-Fi networks:");

    // Set LED to solid blue to indicate networks were found
    setSolidBlue();

    for (int i = 0; i < numNetworks; ++i) {
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (RSSI: ");
        Serial.print(WiFi.RSSI(i));
        Serial.print(") ");
        Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured");
    }
    Serial.println();
}
