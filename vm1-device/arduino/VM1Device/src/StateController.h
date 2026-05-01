#pragma once
#include <Arduino.h>

// Bit positions (semantic state only)
#define ASSIGNED_SHIFT  0
#define PLAYING_SHIFT   1
#define FOCUSED_SHIFT   2

// Masks
#define ASSIGNED_MASK   (1 << ASSIGNED_SHIFT)
#define PLAYING_MASK    (1 << PLAYING_SHIFT)
#define FOCUSED_MASK    (1 << FOCUSED_SHIFT)

#pragma pack(1)
struct DeviceState
{
  uint8_t rotarySensitivity = 5;
  uint8_t bank = 0;
  unsigned char forward = 0;
  unsigned char backward = 0;
  unsigned char fn = 0;
  unsigned char editButtons[8] = {0};
  unsigned char mediaButtons[16] = {0};
  unsigned char mediaButtonsStates[16] = {0};
};
#pragma pack()
extern DeviceState deviceState;