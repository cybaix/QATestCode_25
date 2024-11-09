// QA_Test_Sequence.h

#ifndef QA_TEST_SEQUENCE_H
#define QA_TEST_SEQUENCE_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Declare function to start QA mode
void enterQAMode();
void checkButtonStates();      // Declare checkButtonStates for button handling

// Declare shared variables for LED colors and count
extern uint32_t colors[];
extern int numColors;

#endif // QA_TEST_SEQUENCE_H
