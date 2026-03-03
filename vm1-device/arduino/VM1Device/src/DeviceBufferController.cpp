#include <Arduino.h>
#include "DeviceBufferController.h"

uint8_t buttonBufferIndex = 0;

void addButtonEventToBuffer(ButtonEvent buttonEvent) {
    Serial.printf("AddButtonEventToBuffer: %d, Id: %d\r\n", buttonEvent.eventType, buttonEvent.buttonId);
    if(buttonBufferIndex < BUFFER_SIZE) { // check if buffer is full
        deviceBuffer.buttonEvents[buttonBufferIndex] = buttonEvent;
        buttonBufferIndex++;
    } 
}

void clearEventBuffer()
{
    for(uint8_t i = 0; i < BUFFER_SIZE; ++i){
        deviceBuffer.buttonEvents[i] = {EventType::NO_EVENT, 0};
    }
    buttonBufferIndex = 0;
}