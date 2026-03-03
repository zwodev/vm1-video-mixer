# pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "StateController.h"

#define NEOPIXELS_PIN 22
#define NEOPIXEL_COUNT 33

extern Adafruit_NeoPixel strip;

void initNeoPixels();
int *colorForButtonState(ButtonState state);
uint32_t colorFromArray(int color[3]);
void animateAllNeoPixels();
void updateNeoPixels();



