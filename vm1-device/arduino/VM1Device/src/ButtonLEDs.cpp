#include <Arduino.h>
#include "ButtonLEDs.h"

Adafruit_NeoPixel strip(NEOPIXEL_COUNT, NEOPIXELS_PIN, NEO_GRB + NEO_KHZ800);

int dimmed_divider = 20;
int black[] = {0, 0, 0};
int grey[] = {10, 10, 10};
int white[] = {255, 255, 255};
// int red[] = {255, 0, 0};
// int red_dimmed[] = {red[0] / dimmed_divider,
//                     red[1] / dimmed_divider,
//                     red[2] / dimmed_divider};
// int blue[] = {0, 0, 255};
// int blue_dimmed[] = {blue[0] / dimmed_divider, 
//                    blue[1] / dimmed_divider, 
//                    blue[2] / dimmed_divider};
// int yellow[] = {255, 150, 0};
// int yellow_dimmed[] = {yellow[0] / dimmed_divider,
//                        yellow[1] / dimmed_divider,
//                        yellow[2] / dimmed_divider};
// int orange[] = {255, 137, 79};
// int orange_dimmed[] = {orange[0] / dimmed_divider,
//                        orange[1] / dimmed_divider,
//                        orange[2] / dimmed_divider};
// int green[] = {0, 255, 0};
// int green_dimmed[] = {green[0] / dimmed_divider,
//                       green[1] / dimmed_divider,
//                       green[2] / dimmed_divider};

void initNeoPixels() {
  strip.begin();
  strip.setBrightness(255);
  strip.show();
}

uint32_t colorForButtonState(unsigned char mediaButtonsState)
{
  int color[3] = {0};
  unsigned long currentMillis = millis();
  static unsigned long lastFlash = currentMillis;
  static bool flashState = false;
  unsigned long flashDuration = 10;
  unsigned long flashInterval = 1500;

  if (mediaButtonsState & ASSIGNED_MASK)
  {
    memcpy(color, grey, sizeof(color));
  }

  // todo: how to indicate if active video differs from staged?

  if(mediaButtonsState & FOCUSED_MASK)
  {
    memcpy(color, white, sizeof(color));
  }

  if (mediaButtonsState & PLAYING_MASK)
  {
    float fadeValue = abs(sin(millis() / 500.0f));
    for(int i = 0; i < 3; ++i)
    {
      color[i] *= fadeValue;
    }
  }
 
  return strip.Color(color[0], color[1], color[2]);
}

void startupAnimation()
{
  for (int i = 0; i < NEOPIXEL_COUNT; ++i)
  {
    strip.setPixelColor(i, strip.Color(0,0,0));
  }
  strip.show();

  for (int i = 0; i < NEOPIXEL_COUNT; ++i)
  {
    strip.setPixelColor(i,strip.Color(grey[0], grey[1], grey[2]));
    delay(25);
    strip.show();
  }

  delay(25);

  for (int i = 0; i < NEOPIXEL_COUNT; ++i)
  {
    strip.setPixelColor(i, strip.Color(0,0,0));
    delay(25);
    strip.show();
  }
}


void updateNeoPixels()
{
  // forward-key [0]
  strip.setPixelColor(0, colorForButtonState(deviceState.forward));
  
  // backward-key [1]
  strip.setPixelColor(1, colorForButtonState(deviceState.backward));

  // 8 edit-keys [2-9]
  for (uint8_t i = 0; i < 8; ++i)
  {
    strip.setPixelColor(2 + i, colorForButtonState(deviceState.editButtons[i]));
  }

  // upper row media-keys [10-17]
  for (uint8_t i = 0; i < 8; ++i)
  {
    strip.setPixelColor(10 + i, colorForButtonState(deviceState.mediaButtonsStates[7 - i]));
  }

  // fn-key [18]
  strip.setPixelColor(18, colorForButtonState(deviceState.fn));

  // lower row media-keys [19-26]
  for (uint8_t i = 0; i < 8; ++i)
  {
    strip.setPixelColor(19 + i, colorForButtonState(deviceState.mediaButtonsStates[8 + i]));
  }

  strip.show();
}