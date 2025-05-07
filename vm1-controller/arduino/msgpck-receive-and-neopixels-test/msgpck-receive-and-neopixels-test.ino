#include <Adafruit_NeoPixel.h>
#include "MsgPack/MsgPack.h"

#define LED_PIN 15
#define LED_COUNT 15

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

const int SERIAL_BUF_SIZE = 512;
uint8_t serialBuf[SERIAL_BUF_SIZE];
int serialLen = 0;

void setup()
{
    Serial.begin(115200);
    strip.begin();
    strip.show();
}

void loop()
{
    // Read serial data into buffer
    while (Serial.available() > 0 && serialLen < SERIAL_BUF_SIZE)
    {
        serialBuf[serialLen++] = Serial.read();
        delay(1);
    }

    if (serialLen > 0)
    {
        MsgPack::Unpacker unpacker;
        unpacker.feed(serialBuf, serialLen);

        // Prepare a variable to hold the deserialized data
        MsgPack::arr_t<MsgPack::arr_t<int>> leds;

        // Deserialize directly into the variable
        unpacker.deserialize(leds);

        if (leds.size() == LED_COUNT)
        {
            for (uint32_t i = 0; i < LED_COUNT; i++)
            {
                if (leds[i].size() == 3)
                {
                    int r = leds[i][0];
                    int g = leds[i][1];
                    int b = leds[i][2];
                    strip.setPixelColor(i, strip.Color(r, g, b));
                }
            }
            strip.show();
        }
        serialLen = 0; // Reset buffer
    }
}