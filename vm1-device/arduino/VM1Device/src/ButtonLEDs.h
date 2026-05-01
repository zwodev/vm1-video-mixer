# pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "StateController.h"

#define NEOPIXELS_PIN 22
#define NEOPIXEL_COUNT 33

extern Adafruit_NeoPixel strip;
extern int8_t lastPressedMediaButtonId;

void initNeoPixels();
uint32_t colorForButtonState(unsigned char mediaButtonsState);
void startupAnimation();
void updateNeoPixels();
