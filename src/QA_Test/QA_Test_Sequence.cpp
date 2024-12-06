#include "QA_Test/QA_Test_Sequence.h"
#include "LED/NeoPixelControl.h"
#include <Adafruit_NeoPixel.h>
#include "pins.h"

// Array of LED pins
extern int ledPins[];
extern int numLeds;

// NeoPixel setup
extern Adafruit_NeoPixel statusLED;
extern Adafruit_NeoPixel neoPixels;  // NeoPixel strip on GPIO18
extern uint32_t colors[];
extern int numColors;

// Rotary Encoder variables for manual control
//extern volatile int encoderValue;
int currentLED = 0;
int currentColor = 0;
bool neoPixelOn = true;

void checkButtonStates() {
    // Read button states
    bool buttonEnterPressed = digitalRead(BUTTON_ENTER_PIN) == LOW;
    bool buttonBackPressed = digitalRead(BUTTON_BACK_PIN) == LOW;

    if (buttonEnterPressed) {
        Serial.println("Button_Enter pressed!");
        delay(200);  // Basic debounce delay
    }

    if (buttonBackPressed) {
        Serial.println("Button_Back pressed!");
        delay(200);  // Basic debounce delay
    }
}

void automatedCycle() {
    Serial.println("Automated Cycle Start");

    for (int i = 0; i < numLeds; i++) {
        digitalWrite(ledPins[i], HIGH);
        delay(100);
        digitalWrite(ledPins[i], LOW);
    }

    for (int j = 0; j < numColors; j++) {
        statusLED.setPixelColor(0, colors[j]);
        statusLED.show();
        delay(500);
    }
    statusLED.clear();
    statusLED.show();

    for (int j = 0; j < numColors; j++) {
        cycleNeoPixelColors();
        delay(500);
    }

    clearNeoPixels();

    tone(BUZZER_PIN, 1000);
    delay(300);
    noTone(BUZZER_PIN);

    Serial.println("Automated Cycle End");
}

void enterQAMode() {
    Serial.println("Entering QA Mode");

    automatedCycle();

    //static int lastEncoderValue = 0;
    unsigned long lastActivityTime = millis();  // Track the time of the last activity

    /*
    while (true) {
        checkButtonStates();

        if (encoderValue > lastEncoderValue || encoderValue < lastEncoderValue) {
            lastActivityTime = millis();  // Reset inactivity timer on encoder turn

            if (encoderValue > lastEncoderValue) {
                digitalWrite(ledPins[currentLED], LOW);
                currentLED = (currentLED + 1) % numLeds;
                digitalWrite(ledPins[currentLED], HIGH);
                Serial.print("Current LED: ");
                Serial.println(currentLED);
            } else {
                currentColor = (currentColor + 1) % numColors;
                if (neoPixelOn) {
                    statusLED.setPixelColor(0, colors[currentColor]);
                    statusLED.show();

                    for (int i = 0; i < NUM_NEOPIXELS; i++) {
                        neoPixels.setPixelColor(i, colors[currentColor]);
                    }
                    neoPixels.show();

                    Serial.print("Current Color: ");
                    Serial.println(currentColor);
                }
            }
            lastEncoderValue = encoderValue;
        }

        // Check for inactivity: exit QA Mode if 10 seconds (10,000 ms) have passed
        if (millis() - lastActivityTime > 10000) {
            Serial.println("Exiting QA Mode due to inactivity.");
            clearNeoPixels();       // Turn off NeoPixels on exit
            statusLED.clear();      // Turn off status LED
            statusLED.show();
            break;                  // Exit the QA Mode loop
        }

        delay(10);  // Small delay for stability
    }*/
}
