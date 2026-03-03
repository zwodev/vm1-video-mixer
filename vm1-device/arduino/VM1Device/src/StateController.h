#pragma once
#include <Arduino.h>

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
};
#pragma pack()
extern DeviceState deviceState;