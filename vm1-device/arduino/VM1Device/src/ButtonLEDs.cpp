#include <Arduino.h>
#include "ButtonLEDs.h"

Adafruit_NeoPixel strip(NEOPIXEL_COUNT, NEOPIXELS_PIN, NEO_GRB + NEO_KHZ800);

int dimmed_divider = 20;
int black[] = {0, 0, 0};
int grey[] = {10, 10, 10};
int white[] = {255, 255, 255};
int red[] = {255, 0, 0};
int red_dimmed[] = {red[0] / dimmed_divider,
                    red[1] / dimmed_divider,
                    red[2] / dimmed_divider};
int blue[] = {0, 0, 255};
int blue_dimmed[] = {blue[0] / dimmed_divider, 
                   blue[1] / dimmed_divider, 
                   blue[2] / dimmed_divider};
int yellow[] = {255, 150, 0};
int yellow_dimmed[] = {yellow[0] / dimmed_divider,
                       yellow[1] / dimmed_divider,
                       yellow[2] / dimmed_divider};
int orange[] = {255, 137, 79};
int orange_dimmed[] = {orange[0] / dimmed_divider,
                       orange[1] / dimmed_divider,
                       orange[2] / dimmed_divider};
int green[] = {0, 255, 0};
int green_dimmed[] = {green[0] / dimmed_divider,
                      green[1] / dimmed_divider,
                      green[2] / dimmed_divider};

void initNeoPixels() {
  strip.begin();
  strip.setBrightness(255);
  strip.show();
}

int *colorForButtonState(unsigned char mediaButtonsState)
{
  int color[3] = {0};
  if (mediaButtonsState & ASSIGNED_MASK)
  {
    memcpy(color, grey, sizeof(color));
  }
  if (mediaButtonsState & PLAYING_MASK ||
      mediaButtonsState & FOCUSED_MASK)
  {
    memcpy(color, white, sizeof(color));
  }

  if(mediaButtonsState & FOCUSED_MASK)
  {
    fadeValue = 1.0f - abs(sin(millis()/500.0f)); /// TODO 
  }
  return color;
}

int *colorForButtonState(ButtonState state)
{
  switch (state)
  {
  case NONE:
    return black;
  case EMPTY:
    return grey;
  case FILE_ASSET_ACTIVE:
      return red;
  case FILE_ASSET:
    return red_dimmed;
  case LIVECAM_ACTIVE:
    return orange;
  case LIVECAM:
    return orange_dimmed;
  case SHADER_ACTIVE:
    return yellow;
  case SHADER:
    return yellow_dimmed;
  case MEDIABUTTON_SELECTED:
    return blue;
  case YELLOW:
    return yellow;
  case GREEN:
    return green;
  case BLUE:
    return blue;
  case RED:
    return red;
  default:
    return black;
  }
}

uint32_t colorFromArray(int color[3])
{
  return strip.Color(color[0], color[1], color[2]);
}

void neoPixelsStartAnimation()
{
  for (int i = 0; i < NEOPIXEL_COUNT; ++i)
  {
    strip.setPixelColor(i, colorFromArray(black));
  }
  strip.show();

  for (int i = 0; i < NEOPIXEL_COUNT; ++i)
  {
    strip.setPixelColor(i, colorFromArray(red_dimmed));
    delay(50);
    strip.show();
  }

  delay(50);

  for (int i = 0; i < NEOPIXEL_COUNT; ++i)
  {
    strip.setPixelColor(i, colorFromArray(black));
    delay(50);
    strip.show();
  }
}


void updateNeoPixels()
{
  // forward-key [0]
  int *color = colorForButtonState(deviceState.forward);
  strip.setPixelColor(0, colorFromArray(color));
  
  // backward-key [1]
  color = colorForButtonState(deviceState.backward);
  strip.setPixelColor(1, colorFromArray(color));

  // 8 edit-keys [2-9]
  for (uint8_t i = 0; i < 8; ++i)
  {
    color = colorForButtonState(deviceState.editButtons[i]);
    strip.setPixelColor(2 + i, colorFromArray(color));
  }

  // upper row media-keys [10-17]
  for (uint8_t i = 0; i < 8; ++i)
  {
    // color = colorForButtonState(deviceState.mediaButtons[7 - i]);
    color = colorForButtonState(deviceState.mediaButtonsStates[7 - i]);
    strip.setPixelColor(10 + i, colorFromArray(color));
  }

  // fn-key [18]
  color = colorForButtonState(deviceState.fn);
  strip.setPixelColor(18, colorFromArray(color));

  // lower row media-keys [19-26]
  for (uint8_t i = 0; i < 8; ++i)
  {
    // color = colorForButtonState(deviceState.mediaButtons[8 + i]);
    color = colorForButtonState(deviceState.mediaButtonsStates[8 + i]);
    strip.setPixelColor(19 + i, colorFromArray(color));
  }

  // 6 bank-pixels [27-32]
  for (uint8_t i = 0; i < 6; ++i)
  {
    color = red_dimmed;
    if (i == deviceState.bank)
      color = orange_dimmed;
    strip.setPixelColor(32 - i, colorFromArray(color));
  }

  strip.show();
}

void setMediaButtonLED(uint8_t buttonId, int* color)
{
  if (buttonId < 8){   // upper row
    strip.setPixelColor(10 + (7-buttonId), colorFromArray(color));
  } else {
    strip.setPixelColor(19 + (buttonId-8), colorFromArray(color));
  }
}

// void animateActiveMediaSlotLED()
// {
//   if(lastPressedMediaButtonId <0) return;

//   unsigned long current_millis = millis();  
//   static long time = 0;
//   static float fadeValue = 0.0f;
  
//   if (current_millis - time > 16)
//   {
//     time = current_millis;
//     fadeValue = 1.0f - abs(sin(millis()/500.0f));
//     float divider = 1.0f + fadeValue * (dimmed_divider - 1.0f);
//     int fadeColor[3] = {0,0,0};
//     int* color = colorForButtonState(deviceState.mediaButtons[lastPressedMediaButtonId]);
//     for(int i = 0; i < 3; i++){ 
//       fadeColor[i] = color[i] / (int)divider;
//     }
//     setMediaButtonLED(lastPressedMediaButtonId, fadeColor);
//     strip.show();
//   }
// }