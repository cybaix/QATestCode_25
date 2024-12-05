#include <WiFi.h>
#include <Arduino.h>
//#include <Adafruit_MAX1704X.h>  // Include MAX17048 library
#include "pins.h"
#include <SPI.h>
#include <SD.h>
#include "QA_Test/QA_Test_Sequence.h"
#include "LED/Status_LED.h"
#include "WiFi/WiFi_Settings.h"   // Wi-Fi connection settings
#include "WiFi/WiFi_Module.h"     // Wi-Fi scanning functionality
#include "version.h"
#include "Diagnostics/Diagnostics.h"
#include "BadgePirates/PirateShipAnimation.h"
#include "LED/NeoPixelControl.h"
#include "Screen/Screen_Module.h"



//Adafruit_MAX17048 max17048;  // Create MAX17048 object

//unsigned long lastBatteryCheck = 0;        // Track last battery check time
//const unsigned long batteryCheckInterval = 60000;  // Check battery every 60 seconds

unsigned long lastDiagnosticsRun = 0;      // Track last diagnostics run time
const unsigned long diagnosticsInterval = 600000;  // Run every 10 minutes (600000 ms)

SPIClass hspi = SPIClass(HSPI); // Using HSPI as it's unused

// Array of LED pins
int ledPins[] = {LED_PIN_1, LED_PIN_2, LED_PIN_3, LED_PIN_4, LED_PIN_5, LED_PIN_6};
int numLeds = sizeof(ledPins) / sizeof(ledPins[0]);

// Define color array and number of colors for NeoPixel
uint32_t colors[] = {
    statusLED.Color(255, 0, 0),   // Red
    statusLED.Color(0, 255, 0),   // Green
    statusLED.Color(0, 0, 255)    // Blue
};
int numColors = sizeof(colors) / sizeof(colors[0]);

// Variables for rotary encoder
volatile int lastEncoded = 0;
volatile int encoderValue = 0;

// State tracking for the boot button with debounce
unsigned long lastEnterPressTime = 0;
bool firstEnterPressDetected = false;

void IRAM_ATTR updateEncoder() {
    int MSB = digitalRead(ENCODER_PIN_A);  // Most significant bit
    int LSB = digitalRead(ENCODER_PIN_B);  // Least significant bit

    int encoded = (MSB << 1) | LSB;
    int sum = (lastEncoded << 2) | encoded;

    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue++;
    if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue--;

    lastEncoded = encoded;
}

void printDeviceInfo() {
    // Display code version
    Serial.print("Code Version: ");
    Serial.println(CODE_VERSION);

    // Display ESP MAC address
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());

    // Calculate memory usage
    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t usedHeap = totalHeap - freeHeap;
    float memoryUsagePercent = (float(usedHeap) / float(totalHeap)) * 100;

    // Display memory usage
    Serial.print("Total Heap: ");
    Serial.print(totalHeap);
    Serial.println(" bytes");
    Serial.print("Free Heap: ");
    Serial.print(freeHeap);
    Serial.println(" bytes");
    Serial.print("Used Heap: ");
    Serial.print(usedHeap);
    Serial.print(" bytes (");
    Serial.print(memoryUsagePercent, 2);
    Serial.println("% used)");
}



void setup() {
    Serial.begin(115200);
    while (!Serial);

    // Initialize each LED pin as an output
    for (int i = 0; i < numLeds; i++) {
        pinMode(ledPins[i], OUTPUT);
        digitalWrite(ledPins[i], HIGH);
        Serial.printf("Set LED %d on\n", i);
        delay(500);
    }

    for (int i = numLeds - 1; i > -1; i--) {
        digitalWrite(ledPins[i], LOW);
        Serial.printf("Set LED %d off\n", i);
        delay(500);
    }

    initializeScreen();  // Initialize the TFT screen
    displayWelcomeMessage();  // Display a welcome message

    // Initialize I2C for MAX17048
   // if (!max17048.begin(&Wire)) {
   //     Serial.println("Could not find MAX17048 chip!");
   // } else {
   //     Serial.println("MAX17048 found!");
   // }

    // Initialize buttons with pull-up resistors
    pinMode(BUTTON_ENTER_PIN, INPUT_PULLUP);
    pinMode(BUTTON_BACK_PIN, INPUT_PULLUP);
    
    // Display ESP32 information on startup
    printDeviceInfo();

    // Initialize the NeoPixels LED
    initStatusLED();    // Initialize NeoPixels on GPIO18
    initNeoPixels();

    // Initialize SPI for SD card with custom pins
    hspi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    
    // Initialize SD card
    if (SD.begin(SD_CS, hspi)) {
        Serial.println("SD card initialized successfully.");
        if (SD.cardSize() > 0) {
            Serial.print("SD Card Size: ");
            Serial.print(SD.cardSize() / (1024 * 1024));
            Serial.println(" MB");
        } else {
            Serial.println("SD card detected but unable to determine size.");
        }
    } else {
        Serial.println("No SD card detected. Please insert an SD card.");
    }

    // Initialize rotary encoder pins
    pinMode(ENCODER_PIN_A, INPUT_PULLUP);
    pinMode(ENCODER_PIN_B, INPUT_PULLUP);
    pinMode(ENCODER_BUTTON_PIN, INPUT_PULLUP);

    // Initialize boot button
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

    // Attach interrupts for rotary encoder
    //attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), updateEncoder, CHANGE);
    //attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), updateEncoder, CHANGE);

    // Run an initial Wi-Fi scan and connect to Wi-Fi
    scanWiFiNetworks();     // Use scan function from WiFi_Module
    connectToWiFi();        // Use connect function from WiFi_Settings

    // Display initial battery status
    //checkBatteryStatus();

    // Initialize diagnostics logging
    if (!initDiagnostics()) {
        Serial.println("Error initializing diagnostics.");
    }

    Serial.println("YoHoHo its a Pirates Life for me...");
    playPirateShipAnimation();  // Play the animation on startup
}

void loop() {
    // Enter button double-press detection
    int enterButtonState = digitalRead(BUTTON_ENTER_PIN);
    if (enterButtonState == LOW && !firstEnterPressDetected) {
        firstEnterPressDetected = true;
        lastEnterPressTime = millis();
    } else if (enterButtonState == LOW && firstEnterPressDetected && (millis() - lastEnterPressTime < 500)) {
        // Second press within 500 ms - switch to QA mode
        enterQAMode();  // Enter QA mode
    } else if (millis() - lastEnterPressTime > 500) {
        // Reset if more than 500 ms passed since the first press
        firstEnterPressDetected = false;
    }

    // Run diagnostics every 10 minutes
    if (millis() - lastDiagnosticsRun >= diagnosticsInterval) {
        runDiagnostics();
        lastDiagnosticsRun = millis();
    }
    
   // if (millis() - lastBatteryCheck >= batteryCheckInterval) {
   //     checkBatteryStatus();
   //     lastBatteryCheck = millis();
   // }

    // Call the button check function to handle button actions outside QA Mode
    checkButtonStates();
}
