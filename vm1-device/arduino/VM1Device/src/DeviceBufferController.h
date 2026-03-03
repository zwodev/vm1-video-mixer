#pragma once
#include <Arduino.h>

#define BUFFER_SIZE 8

enum EventType : uint8_t {
  EDIT_BUTTON_EVENT,
  MEDIA_BUTTON_EVENT,
  NAVIGATION_BUTTON_EVENT,
  ROTARY_EVENT,
  NO_EVENT
};

#pragma pack(1)
struct ButtonEvent {
  EventType eventType;
  int8_t buttonId;
};
#pragma pack()

#pragma pack(1)
struct DeviceBuffer
{
  ButtonEvent buttonEvents[BUFFER_SIZE];
  bool fnPressed;
  uint16_t analogInput[4];
};
#pragma pack()
extern DeviceBuffer deviceBuffer;

void clearEventBuffer();
void addButtonEventToBuffer(ButtonEvent buttonEvent);