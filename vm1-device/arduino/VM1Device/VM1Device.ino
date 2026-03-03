#include <stdint.h>
#include <stdio.h>
#include <Wire.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "src/RotaryEncoder.h"
#include "src/ButtonMatrix.h"
#include "src/DeviceBufferController.h"
#include "src/StateController.h"
#include "src/ButtonLEDs.h"

#define I2C_SLAVE_ADDRESS 0x08

// analog inputs
#define A0_PIN 26
#define A1_PIN 27
#define A2_PIN 28
#define A3_PIN 29

DeviceBuffer deviceBuffer = {};   // output buffer for events. Gets cleared after events are sent via i2c
DeviceState deviceState = {};     // state for the neopixel display, rotary encoder sensitivity etc.

void showDebugMessage()
{
  unsigned long current_millis = millis();
  static long time = 0;
  if (current_millis - time > 1000)
  {
    time = current_millis;
    Serial.printf("EventType: %d, ButtonId: %d\r\n", deviceBuffer.buttonEvents[0].eventType, deviceBuffer.buttonEvents[0].buttonId);
    Serial.printf("device buffer size: %d\r\n", sizeof(deviceBuffer));
    Serial.printf("buttonEvent size: %d\r\n", sizeof(ButtonEvent));
    Serial.printf("ShiftKey: %d\r\n", deviceBuffer.fnPressed );
  }
}

void onI2CRequestHandler()
{
  Wire.write(reinterpret_cast<const uint8_t *>(&deviceBuffer), sizeof(deviceBuffer));
  clearEventBuffer();
}

void onI2CReceiveHandler(int numBytes)
{
  if (numBytes == sizeof(deviceState))
  {
    Wire.readBytes(reinterpret_cast<char *>(&deviceState), sizeof(deviceState));
    updateNeoPixels();
  }
}

void setup()
{
  Serial.begin(115200);

  initButtonMatrix();
  initRotaryEncoders();
  initNeoPixels();
  clearEventBuffer();

  Wire.setSDA(20); // GP20 = SDA
  Wire.setSCL(21); // GP21 = SCL
  Wire.begin(I2C_SLAVE_ADDRESS);
  Wire.onRequest(onI2CRequestHandler);
  Wire.onReceive(onI2CReceiveHandler);

  animateAllNeoPixels();
}

void loop()
{
  updateButtonMatrix();
  updateRotaryEncoders();

  uint16_t analog0 = analogRead(A0_PIN);  // ToDo: filter analog inputs, add all four of them
  deviceBuffer.analogInput[0] = analog0;
  
  // showDebugMessage();
}
