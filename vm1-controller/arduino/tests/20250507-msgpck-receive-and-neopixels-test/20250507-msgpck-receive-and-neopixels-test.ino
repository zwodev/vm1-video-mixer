#include <Adafruit_NeoPixel.h>
#include "MsgPack/MsgPack.h"

#define NEOPIXELS_PIN 22
#define NEOPIXEL_COUNT 27

Adafruit_NeoPixel strip(NEOPIXEL_COUNT, NEOPIXELS_PIN, NEO_GRB + NEO_KHZ800);

const int SERIAL_BUF_SIZE = 512;
uint8_t serialBuf[SERIAL_BUF_SIZE];
int serialLen = 0;

enum button_state
{
    NONE,
    EMPTY,
    FILE_ASSET,
    LIVECAM,
    SHADER,
    FILE_ASSET_ACTIVE,
    LIVECAM_ACTIVE,
    SHADER_ACTIVE,
};

int dimmed_divider = 20;

int black[] = {0, 0, 0};
int grey[] = {10, 10, 10};
// int grey[] = {9, 9, 9};
int red[] = {255, 0, 0};
int dark_red[] = {255 / dimmed_divider, 0, 0};
int green[] = {0, 255, 0};
int dark_green[] = {0, 255 / dimmed_divider, 0};
int blue[] = {0, 0, 255};
int dark_blue[] = {0, 0, 255 / dimmed_divider};
int white[] = {255, 255, 255};
int yellow[] = {255, 255, 0};
int magenta[] = {255, 0, 255};
int cyan[] = {0, 255, 255};

int *colorForButtonState(button_state state)
{
    switch (state)
    {
    case NONE:
        return black;
    case EMPTY:
        return grey;

    case FILE_ASSET:
        return dark_green;
    case FILE_ASSET_ACTIVE:
        return green;

    case LIVECAM:
        return dark_red;
    case LIVECAM_ACTIVE:
        return red;

    case SHADER:
        return dark_blue;
    case SHADER_ACTIVE:
        return blue;

    default:
        return black;
    }
}

uint32_t colorFromArray(int color[3])
{
    return strip.Color(color[0], color[1], color[2]);
}

void setup()
{
    Serial.begin(115200);
    strip.begin();
    strip.setBrightness(25);
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

        MsgPack::arr_t<int> states;

        if (unpacker.deserialize(states) && states.size() == NEOPIXEL_COUNT)
        {
            for (uint32_t i = 0; i < NEOPIXEL_COUNT; i++)
            {
                int *color = colorForButtonState(static_cast<button_state>(states[i]));
                strip.setPixelColor(i, colorFromArray(color));
            }
            strip.show();
        }

        serialLen = 0; // Reset buffer
    }
}