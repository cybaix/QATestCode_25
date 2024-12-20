#include "Screen/Screen_Module.h"
#include <lvgl.h>
#include "pins.h"

TFT_eSPI tft = TFT_eSPI();  // Create TFT instance

void initializeScreen() {
    Serial.println("Initializing screen...");
    tft.init();
    tft.setSwapBytes(true);
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);  // Clear the screen
    
    //pinMode(TFT_LED, OUTPUT);  // Configure backlight pin
    //digitalWrite(TFT_LED, HIGH);  // Turn on backlight

    Serial.println("Screen initialized successfully!");
}

void displayWelcomeMessage() {
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(20, 40);
    tft.println("Welcome to the ESP32!");
}

void displayCoordinates(uint x, uint y) {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 40);
    tft.printf("X: %d, Y: %d\n", x, y);
}