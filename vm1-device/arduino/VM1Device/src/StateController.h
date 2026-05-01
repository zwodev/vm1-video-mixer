#pragma once
#include <Arduino.h>

// Bit positions (semantic state only)
#define ASSIGNED_SHIFT     0
#define PLAYING_SHIFT   1
#define FOCUSED_SHIFT   2

// Masks
#define ASSIGNED_MASK   (1 << ASSIGNED_SHIFT)
#define PLAYING_MASK    (1 << PLAYING_SHIFT)
#define FOCUSED_MASK    (1 << FOCUSED_SHIFT)

enum ButtonState : uint8_t
{
  NONE,
  EMPTY,
  FILE_ASSET,
  LIVECAM,
  SHADER,
  FILE_ASSET_ACTIVE,
  LIVECAM_ACTIVE,
  SHADER_ACTIVE,  
  MEDIABUTTON_SELECTED,
  YELLOW,
  GREEN,
  BLUE,
  RED
};

// enum Vm1Color : uint8_t {
//   VM1_BLACK,
//   VM1_SILVER,
//   VM1_GRAY,
//   VM1_WHITE,
//   VM1_MAROON,
//   VM1_RED,
//   VM1_PURPLE,
//   VM1_FUCHSIA,
//   VM1_GREEN,
//   VM1_LIME,
//   VM1_OLIVE,
//   VM1_YELLOW,
//   VM1_NAVY,
//   VM1_BLUE,
//   VM1_TEAL,
//   VM1_AQUA
// };

// enum Vm1Brightness : uint8_t {
//   VM1_BRIGHT_0,
//   VM1_BRIGHT_1,
//   VM1_BRIGHT_2,
//   VM1_BRIGHT_3
// };

#pragma pack(1)
struct DeviceState
{
  uint8_t rotarySensitivity = 5;
  uint8_t bank = 0;
  ButtonState forward = ButtonState::NONE;
  ButtonState backward = ButtonState::NONE;
  ButtonState fn = ButtonState::NONE;
  ButtonState editButtons[8] = {ButtonState::NONE};
  ButtonState mediaButtons[16] = {ButtonState::NONE};
  unsigned char mediaButtonsStates[16] = {0};
};
#pragma pack()
extern DeviceState deviceState;