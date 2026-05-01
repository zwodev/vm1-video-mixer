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
#include "src/AverageFilter.h"

#define I2C_SLAVE_ADDRESS 0x08

volatile uint8_t i2cBuffer[sizeof(DeviceState)];
volatile uint8_t i2cIndex = 0;
volatile bool i2cFrameReady = false;

unsigned long current_millis;
unsigned long millis_old = 0;


// analog inputs
const uint8_t analogInputPins[] = {26, 27, 28, 29};
AverageFilter analogInputFilters[ANALOG_INPUTS_COUNT];

DeviceBuffer deviceBuffer = {};   // output buffer for events. Gets cleared after events are sent via i2c
DeviceState deviceState = {};     // state for the neopixel display, rotary encoder sensitivity etc.
int8_t lastPressedMediaButtonId = -1;

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
    while (Wire.available())
    {
        if (i2cIndex < sizeof(DeviceState))
        {
            i2cBuffer[i2cIndex++] = Wire.read();
        }
        else
        {
            Wire.read(); // discard overflow
        }
    }

    // frame complete?
    if (i2cIndex >= sizeof(DeviceState))
    {
        i2cFrameReady = true;
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

  neoPixelsStartAnimation();
}

void loop()
{
  current_millis = millis();
  
  updateButtonMatrix();
  updateRotaryEncoders();

  if (i2cFrameReady)
  {
      noInterrupts();  // protect shared buffer flag

      memcpy(&deviceState,
              (void*)i2cBuffer,
              sizeof(DeviceState));

      i2cIndex = 0;
      i2cFrameReady = false;

      interrupts();

      // DEBUG OUTPUT
      Serial.println("Received DeviceState:");

      for (int i = 0; i < 16; i++)
      {
          Serial.printf("[%d] ", i);

          uint8_t s = deviceState.mediaButtonsStates[i];

          if (s & ASSIGNED_MASK) Serial.print("ASSIGNED ");
          if (s & PLAYING_MASK)  Serial.print("PLAYING ");
          if (s & FOCUSED_MASK)  Serial.print("FOCUSED ");

          if (s == 0) Serial.print("EMPTY");

          Serial.println();
      }
  }

 
  if (current_millis - millis_old > 16)
  {
    millis_old = current_millis;
    updateNeoPixels();
  }
  // animateActiveMediaSlotLED();

  for(int i = 0; i < ANALOG_INPUTS_COUNT; ++i)
  {
    float analogIn = analogRead(analogInputPins[i]);
    deviceBuffer.analogInput[i] = analogInputFilters[i].Filter(analogIn);    
  }
  
  // showDebugMessage();
}
