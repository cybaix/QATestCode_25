#ifndef SCREEN_MODULE_H
#define SCREEN_MODULE_H

#include <TFT_eSPI.h>  // TFT library

// Function declarations
void initializeScreen();
void displayWelcomeMessage();
void displayCoordinates(uint x, uint y);

#endif  // SCREEN_MODULE_H
