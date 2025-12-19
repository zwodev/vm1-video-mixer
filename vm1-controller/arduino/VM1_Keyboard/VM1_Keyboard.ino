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

int press_up = 0;
int press_down = 0;

// analog inputs
uint16_t analog0 = 0;
uint16_t analog0_old = 0;

// NeoPixels
Adafruit_NeoPixel strip(NEOPIXEL_COUNT, NEOPIXELS_PIN, NEO_GRB + NEO_KHZ800);
int dimmed_divider = 20;
int black[] = {0, 0, 0};
int grey[] = {10, 10, 10};
int white[] = {255, 255, 255};
int red[] = {255, 0, 0};
int red_dimmed[] = {red[0] / dimmed_divider,
                    red[1] / dimmed_divider,
                    red[2] / dimmed_divider};
int blue[] = {0, 0, 255};
int blue_dimmed[] = {blue[0] / dimmed_divider, 
                   blue[1] / dimmed_divider, 
                   blue[2] / dimmed_divider};
int yellow[] = {255, 150, 0};
int yellow_dimmed[] = {yellow[0] / dimmed_divider,
                       yellow[1] / dimmed_divider,
                       yellow[2] / dimmed_divider};
int orange[] = {255, 137, 79};
int orange_dimmed[] = {orange[0] / dimmed_divider,
                       orange[1] / dimmed_divider,
                       orange[2] / dimmed_divider};


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

DeviceState deviceState;

#pragma pack(1)
struct DeviceBuffer
{
  char buttons[8];
  bool shiftPressed;
  int32_t encoder0;
  int32_t encoder1;
  uint16_t analog0;
};
#pragma pack()

DeviceBuffer deviceBuffer = {};

// shared debug message between both cores
// volatile char debug_msg[32] = "- no info -";
// volatile bool debug_msg_ready = false;

void blink() // helper function for anything, e.g. debug message
{
  unsigned long current_millis = millis();
  static long time = 0;

  if (current_millis - time > 1000)
  {
    time = current_millis;

    // print_keyboard_buffer();
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

  case FILE_ASSET_ACTIVE:
      return red;
  case FILE_ASSET:
    return red_dimmed;

  case LIVECAM_ACTIVE:
    return orange;
  case LIVECAM:
    return orange_dimmed;

  case SHADER_ACTIVE:
    return yellow;
  case SHADER:
    return yellow_dimmed;

  case MEDIABUTTON_SELECTED:
    return blue_dimmed;

  default:
    return black;
  }
}

uint32_t colorFromArray(int color[3])
{
  return strip.Color(color[0],
                     color[1],
                     color[2]);
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
    strip.setPixelColor(i, colorFromArray(red_dimmed));
    delay(25);
    strip.show();
  }

  delay(250);

  for (int i = 0; i < NEOPIXEL_COUNT; ++i)
  {
    strip.setPixelColor(i, colorFromArray(black));
  }
  strip.show();
}

void onI2CRequestHandler()
{
  Wire.write(reinterpret_cast<const uint8_t *>(&deviceBuffer), sizeof(deviceBuffer));
  clear_keyboard_buffer();
}

void onI2CReceiveHandler(int numBytes)
{
  if (numBytes == sizeof(deviceState))
  {
    Wire.readBytes(reinterpret_cast<char *>(&deviceState), sizeof(deviceState));
    setNeoPixels();
  }
}

void setNeoPixels()
{
  // forward-key [0]
  int *color = colorForButtonState(deviceState.forward);
  strip.setPixelColor(0, colorFromArray(color));
  
  // backward-key [1]
  color = colorForButtonState(deviceState.backward);
  strip.setPixelColor(1, colorFromArray(color));

  // 8 edit-keys [2-9]
  for (uint8_t i = 0; i < 8; ++i)
  {
    color = colorForButtonState(deviceState.editButtons[i]);
    strip.setPixelColor(2 + i, colorFromArray(color));
  }

  // upper row media-keys [10-17]
  for (uint8_t i = 0; i < 8; ++i)
  {
    color = colorForButtonState(deviceState.mediaButtons[7 - i]);
    strip.setPixelColor(10 + i, colorFromArray(color));
  }

  // fn-key [18]
  color = colorForButtonState(deviceState.fn);
  strip.setPixelColor(18, colorFromArray(color));

  // lower row media-keys [19-26]
  for (uint8_t i = 0; i < 8; ++i)
  {
    color = colorForButtonState(deviceState.mediaButtons[8 + i]);
    strip.setPixelColor(19 + i, colorFromArray(color));
  }

  // 6 bank-pixels [27-32]
  for (uint8_t i = 0; i < 6; ++i)
  {
    color = red_dimmed;
    if (i == deviceState.bank)
      color = orange_dimmed;
    strip.setPixelColor(32 - i, colorFromArray(color));
  }

  strip.show();
}

void setup()
{
  Serial.begin(115200);

  // hid-keyboard
  init_keyboard();
  set_keyboard_buffer(deviceBuffer.buttons, sizeof(deviceBuffer.buttons), &deviceBuffer.shiftPressed);

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
      press_down = 0;
      press_up++;
      if(press_up % deviceState.rotarySensitivity == 0) { 
        res += ", down";
        add_to_keyboard_buffer(KEY_UP_ARROW);
        Keyboard.press(KEY_UP_ARROW);
        // todo: add keycode to keyboard buffer ('KEY_UP_ARROW')
        delay(1);
        Keyboard.release(KEY_UP_ARROW);
      }
    }
    else if (enc0 > 0)
    {
      press_up = 0;
      press_down++;
      if(press_down % deviceState.rotarySensitivity == 0) {
        res += ", up";
        add_to_keyboard_buffer(KEY_DOWN_ARROW);
        Keyboard.press(KEY_DOWN_ARROW);
        // todo: add keycode to keyboard buffer ('KEY_DOWN_ARROW')
        delay(1);
        Keyboard.release(KEY_DOWN_ARROW);
      }
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
      // todo: add keycode to keyboard buffer ('+')
      res += ", down";
    }
    else if (enc1 > 0)
    {
      // todo: add keycode to keyboard buffer ('-')
      res += ", up";
    }
    Serial.println(res);
  }

  analog0 = analogRead(A0_PIN);
  deviceBuffer.analog0 = analog0;
  if (abs(analog0 - analog0_old) > 10) {
    Serial.print("A0: ");
    Serial.println(analog0);
    analog0_old = analog0;
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
  blink();
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
    Serial.print(deviceState.editButtons[i]);
    Serial.print("  ");
  }
  Serial.println();

  // 16 media-keys
  Serial.print("Media-keys:\t");
  for (uint8_t i = 0; i < 16; i++)
  {
    Serial.print(deviceState.mediaButtons[i]);
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

void print_keyboard_buffer()
{
  Serial.print("ButtonBuffer: ");
  for (int i = 0; i < sizeof(deviceBuffer.buttons); ++i)
  {
    Serial.print(deviceBuffer.buttons[i]);
    Serial.print("(");
    Serial.print((uint8_t)deviceBuffer.buttons[i]);
    Serial.print(")");
    if (i < (sizeof(deviceBuffer.buttons) - 1))
      Serial.print("\t");
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