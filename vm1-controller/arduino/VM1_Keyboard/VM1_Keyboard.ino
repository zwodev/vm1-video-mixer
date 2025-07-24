#include <stdint.h>
#include <Adafruit_NeoPixel.h>
#include <stdio.h>
#include <Wire.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "RotaryEncoder.h"
#include "KeyboardMatrix.h"

#define I2C_SLAVE_ADDRESS 0x08

// rotary encoder pins
#define ROTARY_0_PIN_A 19
#define ROTARY_0_PIN_B 18
#define ROTARY_1_PIN_A 17
#define ROTARY_1_PIN_B 16

// neopixel
#define NEOPIXELS_PIN 22
#define NEOPIXEL_COUNT 33

// analog inputs
#define A0_PIN 26
#define A1_PIN 27
#define A2_PIN 28
#define A3_PIN 29

// #define DEBUG
#ifdef DEBUG
uint32_t d_current_micros, d_previous_micros;
uint32_t d_debug_log_interval_micros = 1000000;
uint32_t d_main_loop_duration_micros;
#endif

// encoder
rotary_encoder_t encoder0 = {.pin_a = ROTARY_0_PIN_A, .pin_b = ROTARY_0_PIN_B};
rotary_encoder_t encoder1 = {.pin_a = ROTARY_1_PIN_A, .pin_b = ROTARY_1_PIN_B};
int32_t encoder0_position;
int32_t encoder1_position;

// NeoPixels
Adafruit_NeoPixel strip(NEOPIXEL_COUNT, NEOPIXELS_PIN, NEO_GRB + NEO_KHZ800);
int dimmed_divider = 20;
int black[] = {0, 0, 0};
int grey[] = {10, 10, 10};
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
};

#pragma pack(1)
struct DeviceState
{
  uint8_t bank;
  ButtonState forward = ButtonState::NONE;
  ButtonState backward = ButtonState::NONE;
  ButtonState fn = ButtonState::NONE;
  ButtonState edit[8] = {ButtonState::NONE};
  ButtonState media[16] = {ButtonState::NONE};
};
#pragma pack()

DeviceState deviceState;

#pragma pack(1)
struct DeviceBuffer
{
  char buttons[8];
  int32_t encoder0;
  int32_t encoder1;
};
#pragma pack()

DeviceBuffer deviceBuffer;

// shared debug message between both cores
// volatile char debug_msg[32] = "- no info -";
// volatile bool debug_msg_ready = false;


void blink() // helper function vor anything, e.g. debug message
{
  unsigned long current_millis = millis();
  static long time = 0;

  if (current_millis - time > 250)
  {
    time = current_millis;
  }
}

int *colorForButtonState(ButtonState state)
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
  return 0;
}

void animateAllNeoPixels()
{
  for (int i = 0; i < NEOPIXEL_COUNT; ++i)
  {
    strip.setPixelColor(i, colorFromArray(black));
  }
  strip.show();

  for (int i = 0; i < NEOPIXEL_COUNT; ++i)
  {
    strip.setPixelColor(i, colorFromArray(red));
    delay(100);
    strip.show();
  }

  delay(500);

  for (int i = 0; i < NEOPIXEL_COUNT; ++i)
  {
    strip.setPixelColor(i, colorFromArray(black));
  }
  strip.show();
}

void onI2CRequestHandler()
{
  // char buffer[32] = "";
  // snprintf(buffer, sizeof(buffer), "encoder 1: %ld", encoder0_position);
  // Wire.write(buffer);
  Wire.write(reinterpret_cast<const uint8_t*>(&deviceBuffer), sizeof(deviceBuffer));
}

void onI2CReceiveHandler(int numBytes) {
    
     if (numBytes == sizeof(deviceState)) {
        Wire.readBytes(reinterpret_cast<char*>(&deviceState), sizeof(deviceState));
        setNeoPixels();
    }
}

void setNeoPixels()
{
    print_controller_state();

    // set neopixel colors

    // backward-key
    int *color = colorForButtonState(deviceState.backward);
    strip.setPixelColor(1, colorFromArray(color));

    // forward-key
    color = colorForButtonState(deviceState.forward);
    strip.setPixelColor(0, colorFromArray(color));

    // 8 edit-keys
    for (uint8_t i = 0; i < 8; ++i)
    {
      color = colorForButtonState(deviceState.edit[i]);
      strip.setPixelColor(2 + i, colorFromArray(color));
    }

    // upper row media-keys
    for (uint8_t i = 0; i < 8; ++i)
    {
      color = colorForButtonState(deviceState.media[8 - i]);
      strip.setPixelColor(9 + i, colorFromArray(color));
    }
    
    // fn-key
    color = colorForButtonState(deviceState.fn);
    strip.setPixelColor(18, colorFromArray(color));

    // lower row media-keys
    for (uint8_t i = 0; i < 8; ++i)
    {
      color = colorForButtonState(deviceState.media[i + 8]);
      strip.setPixelColor(19 + i, colorFromArray(color));
    }
    
    // 6 bank-pixels (index 27-32)
    for (uint8_t i = 0; i < 6; ++i)
    {
      color = dark_red;
      if (i == deviceState.bank) 
        color = red;
      strip.setPixelColor(32 - i, colorFromArray(color));
    }
    
    strip.show();
}

void setup()
{
  Serial.begin(115200);

  // hid-keyboard
  init_keyboard();

  // init encoder
  rotary_encoder_init(&encoder0);
  rotary_encoder_init(&encoder1);

  // neopixels
  strip.begin();
  strip.setBrightness(255);
  strip.show();

  // I2C
  Wire.setSDA(20); // GP20 = SDA
  Wire.setSCL(21); // GP21 = SCL
  Wire.begin(I2C_SLAVE_ADDRESS);
  Wire.onRequest(onI2CRequestHandler);
  Wire.onReceive(onI2CReceiveHandler);

  animateAllNeoPixels();
#ifdef DEBUG
  d_current_micros = 0;
  d_previous_micros = 0;
#endif
}

void loop()
{
#ifdef DEBUG
  d_current_micros = micros();
#endif

  update_keyboard();

  // Rotary Encoder(send to Keyboard as UP / DOWN Keys)
  int8_t enc0 = 0;
  if (enc0 = rotary_encoder_process(&encoder0, encoder0_position))
  {
    deviceBuffer.encoder0 = encoder0_position;

    String res = "Encoder 0: ";
    res += encoder0_position;
    if (enc0 < 0)
    {
      res += ", down";

      Keyboard.press(KEY_UP_ARROW);
      delay(1);
      Keyboard.release(KEY_UP_ARROW);
    }
    else if (enc0 > 0)
    {
      res += ", up";

      Keyboard.press(KEY_DOWN_ARROW);
      delay(1);
      Keyboard.release(KEY_DOWN_ARROW);
    }
    Serial.println(res);
  }

  int8_t enc1 = 0;
  if (enc1 = rotary_encoder_process(&encoder1, encoder1_position))
  {
    deviceBuffer.encoder1 = encoder1_position;

    String res = "Encoder 1: ";
    res += encoder1_position;
    if (enc1 < 0)
    {
      res += ", down";
    }
    else if (enc1 > 0)
    {
      res += ", up";
    }
    Serial.println(res);
  }


  // Read Serial and set NeoPixels
  if (Serial.available() >= sizeof(deviceState))
  {
    Serial.readBytes((char *)&deviceState, sizeof(deviceState));

    Serial.println("Received DeviceState from Serial");
    setNeoPixels();
  }


#ifdef DEBUG
  uint32_t d_time_after_main_loop = micros();
  d_main_loop_duration_micros = d_time_after_main_loop - d_current_micros;
  // check how long the main loop takes
  // (hint/todo: it actually should average the time of all the
  //             iterations it does up to the next debug output)
  if (d_current_micros - d_previous_micros > d_debug_log_interval_micros)
  {
    // Serial.print("Main loop duration in us:");
    // Serial.println(d_main_loop_duration_micros);
    if (debug_msg_ready)
    {
      Serial.print("message from core 1: ");
      Serial.println((const char *)debug_msg);
      debug_msg_ready = false;
    }
    Serial.print(".");

    d_previous_micros = d_current_micros;
  }
#endif
}

void print_controller_state()
{
  Serial.println();
  // set bank indicator led
  Serial.print("Bank:\t\t");
  Serial.println(deviceState.bank);

  // backward-key
  Serial.print("Backward-key:\t");
  Serial.println(deviceState.backward);

  // forward-key
  Serial.print("Forward-key:\t");
  Serial.println(deviceState.forward);

  // fn-key
  Serial.print("Fn-key:\t\t");
  Serial.println(deviceState.fn);

  // 8 edit-keys
  Serial.print("Edit-keys:\t");
  for (uint8_t i = 0; i < 8; i++)
  {
    Serial.print(deviceState.edit[i]);
    Serial.print("  ");
  }
  Serial.println();

  // 16 media-keys
  Serial.print("Media-keys:\t");
  for (uint8_t i = 0; i < 16; i++)
  {
    Serial.print(deviceState.media[i]);
    if (i != 7)
    {
      Serial.print("  ");
    }
    else
    {
      Serial.println();
      Serial.print("\t");
      Serial.print("\t");
    }
  }
  Serial.println();
}

// void set_debug_msg_core1(const char *message)
// {
//   snprintf((char *)debug_msg, sizeof(debug_msg), "%s", message); // Safe string copy into the buffer
//   debug_msg_ready = true;                                        // Set flag to indicate the message is ready
// }

// void setup1()
// {
//   set_debug_msg_core1("hello from core 1");
// }

// void loop1()
// {
// }