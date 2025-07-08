#include <stdint.h>
#include <Adafruit_NeoPixel.h>
#include <stdio.h>
#include <Wire.h>

#include "pico/stdlib.h"

// #include "hardware/pio.h"
#include "hardware/gpio.h"
#include "stdlib.h"
#include "Keyboard.h"
#include "rotatorio.h"

#define I2C_SLAVE_ADDRESS 0x08

// input rows pins
#define ROW1 6
#define ROW2 5
#define ROW3 4

// output colums pins
#define COL1 7
#define COL2 8
#define COL3 9
#define COL4 10
#define COL5 11
#define COL6 12
#define COL7 13
#define COL8 14
#define COL9 15

// rotary encoder pins
#define ROTARY_0_PIN_A 17
#define ROTARY_0_PIN_B 16
#define ROTARY_1_PIN_A 19
#define ROTARY_1_PIN_B 18

// neopixel
#define NEOPIXELS_PIN 22
#define NEOPIXEL_COUNT 33

// analog inputs
#define A0_PIN 26
#define A1_PIN 27
#define A2_PIN 28
#define A3_PIN 29

// debug
// #define DEBUG
#ifdef DEBUG
uint32_t d_current_micros, d_previous_micros;
uint32_t d_debug_log_interval_micros = 1000000;
uint32_t d_main_loop_duration_micros;
#endif

// button-matrix
#define PRESSED 0
#define RELEASED 1

const uint8_t NUM_ROWS = 3;
const uint8_t NUM_COLS = 9;
uint8_t rowPins[NUM_ROWS] = {ROW3, ROW2, ROW1};
uint8_t colPins[NUM_COLS] = {COL9, COL8, COL7, COL6, COL5, COL4, COL3, COL2, COL1};
bool current_keyboard_matrix[NUM_ROWS][NUM_COLS] = {false};
bool previous_keyboard_matrix[NUM_ROWS][NUM_COLS] = {false};
char keymap[NUM_ROWS][NUM_COLS] = {
    {KEY_LEFT_ARROW, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i'},
    {KEY_RIGHT_ARROW, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k'},
    {KEY_LEFT_SHIFT, 'z', 'x', 'c', 'v', 'b', 'n', 'm', ','},
};
char last_button = '\0';
bool last_button_state = RELEASED;

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

// enum StatusLedState : uint8_t
// {
//   RUNNING,
//   LED_0_ON,
//   LED_1_ON,
//   LED_2_ON,
//   LED_3_ON,
// };
// StatusLedState led_state = RUNNING;

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
struct ControllerState
{
  uint8_t bank;
  ButtonState forward = ButtonState::NONE;
  ButtonState backward = ButtonState::NONE;
  ButtonState fn = ButtonState::NONE;
  ButtonState edit[8] = {ButtonState::NONE};
  ButtonState media[16] = {ButtonState::NONE};
};
#pragma pack()

// shared debug message between both cores
// volatile char debug_msg[32] = "- no info -";
// volatile bool debug_msg_ready = false;

void check_keyboard_matrix()
{
  for (uint8_t i = 0; i < NUM_ROWS; i++)
  {
    for (uint8_t j = 0; j < NUM_COLS; j++)
    {
      if (previous_keyboard_matrix[i][j] != current_keyboard_matrix[i][j])
      {
        last_button = keymap[i][j];
        last_button_state = !current_keyboard_matrix[i][j];
        return;
      }
    }
  }
  last_button = '\0';
}

void update_keyboard()
{
  for (uint8_t i = 0; i < NUM_COLS; i++)
  {
    gpio_put(colPins[i], true);
    sleep_us(10);

    uint32_t gpio_state = gpio_get_all();
    uint8_t row_values = (gpio_state >> 4) & 0b0111;

    current_keyboard_matrix[2][i] = (row_values & 0b0001) == 0b0001;
    current_keyboard_matrix[1][i] = (row_values & 0b0010) == 0b0010;
    current_keyboard_matrix[0][i] = (row_values & 0b0100) == 0b0100;

    gpio_put(colPins[i], false);
    sleep_us(10);
  }

  check_keyboard_matrix();

  if (last_button != '\0')
  {
    // Serial.print(last_button);
    if (last_button_state == PRESSED)
    {
      Keyboard.press(last_button);
      // Serial.print("Pressed\n");
    }
    else
    {
      Keyboard.release(last_button);
      // Serial.print("Released\n");
    }
  }

  // store current keyboard matrix
  for (uint8_t i = 0; i < NUM_ROWS; i++)
  {
    for (uint8_t j = 0; j < NUM_COLS; j++)
    {
      previous_keyboard_matrix[i][j] = current_keyboard_matrix[i][j];
    }
  }
}

void set_status_led(uint8_t led_index)
{
  // todo: set neopixel bank indicator
}

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

// const int8_t encoder_table[16] = {
//     0, -1, 1, 0,
//     1, 0, 0, -1,
//     -1, 0, 0, 1,
//     0, 1, -1, 0};

// volatile uint8_t last_state = 0;

// int8_t ReadEncoderDelta()
// {
//   uint8_t pin_a = gpio_get(ROTARY_PIN_A);
//   uint8_t pin_b = gpio_get(ROTARY_PIN_B);
//   uint8_t current_state = (pin_a << 1) | pin_b;

//   uint8_t index = (last_state << 2) | current_state;
//   int8_t delta = encoder_table[index];

//   last_state = current_state;
//   return delta;
// }

void IRQCallback(uint gpio, uint32_t events)
{
  // int8_t delta = ReadEncoderDelta();
  // m_iEncoderOffset += delta;
}

void onI2CRequestHandler()
{
  char buffer[32];
  // snprintf(buffer, sizeof(buffer), "encoder 1: %ld", encoder_index);
  Wire.write(buffer);
}

void setup()
{
  Serial.begin(115200);

  // init button-matrix
  for (uint8_t i = 0; i < NUM_ROWS; i++)
  {
    gpio_init(rowPins[i]);
    gpio_set_dir(rowPins[i], GPIO_IN);
  }
  for (uint8_t i = 0; i < NUM_COLS; i++)
  {
    gpio_init(colPins[i]);
    gpio_set_dir(colPins[i], GPIO_OUT);
    gpio_put(colPins[i], false);
  }

  // hid-keyboard
  Keyboard.begin();

  // init encoder
  rotary_encoder_init(&encoder0);
  rotary_encoder_init(&encoder1);

  // neopixels
  strip.begin();
  strip.setBrightness(25);
  strip.show();

  // I2C
  Wire.setSDA(20); // GP20 = SDA
  Wire.setSCL(21); // GP21 = SCL
  Wire.begin(I2C_SLAVE_ADDRESS);
  Wire.onRequest(onI2CRequestHandler);

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
    String res = "Encoder 0: ";
    res += encoder0_position;
    if (enc0 < 0)
    {
      res += ", down";
    }
    else if (enc0 > 0)
    {
      res += ", up";
    }
    Serial.println(res);
  }

  int8_t enc1 = 0;
  if (enc1 = rotary_encoder_process(&encoder1, encoder1_position))
  {
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

  /*
    encoder_value = m_iEncoderOffset;
    int delta = encoder_value - encoder_value_old;

    if (delta >= 4)
    {
      Keyboard.press(KEY_UP_ARROW);
      delay(1);
      Keyboard.release(KEY_UP_ARROW);
      encoder_value_old += 4;
      // Serial.println(encoder_value_old);
    }
    else if (delta <= -4)
    {
      Keyboard.press(KEY_DOWN_ARROW);
      delay(1);
      Keyboard.release(KEY_DOWN_ARROW);
      encoder_value_old -= 4;
      // Serial.println(encoder_value_old);
    }
  */

  // Read Serial and set NeoPixels
  if (Serial.available() >= sizeof(ControllerState))
  {
    ControllerState controllerState;
    Serial.readBytes((char *)&controllerState, sizeof(ControllerState));

    // Serial.println();
    // Serial.println("**** Content of states ****");
    // Serial.print("backward button: ");
    // Serial.println(controllerState.backward);
    // Serial.print("forward button: ");
    // Serial.println(controllerState.forward);
    // Serial.print("fn button: ");
    // Serial.println(controllerState.fn);
    // Serial.println("edit buttons:");
    // for (uint8_t i = 0; i < 8; i++)
    // {
    //   Serial.printf("[%d] %d - ", i, controllerState.edit[i]);
    // }
    // Serial.printf("\r\nmedia buttons:\r\n");
    // for (uint8_t i = 0; i < 16; i++)
    // {
    //   Serial.printf("[%d] %d - ", i, controllerState.media[i]);
    // }
    // Serial.println();

    // print_controller_state(controllerState);

    // set bank indicator led
    set_status_led(controllerState.bank);

    // set neopixel colors

    // backward-key
    int *color = colorForButtonState(controllerState.backward);
    strip.setPixelColor(0, colorFromArray(color));

    // forward-key
    color = colorForButtonState(controllerState.forward);
    strip.setPixelColor(1, colorFromArray(color));

    // fn-key
    color = colorForButtonState(controllerState.fn);
    strip.setPixelColor(18, colorFromArray(color));

    // 8 edit-keys
    for (uint8_t i = 0; i < 8; i++)
    {
      color = colorForButtonState(controllerState.edit[i]);
      strip.setPixelColor(2 + i, colorFromArray(color));
    }

    // 16 media-keys
    for (uint8_t i = 0; i < 16; i++)
    {
      color = colorForButtonState(controllerState.media[i]);
      if (i < 8)
        strip.setPixelColor(17 - i, colorFromArray(color));
      else
        strip.setPixelColor(19 + i - 8, colorFromArray(color));
    }

    // todo: 6 bank-pixels

    strip.show();
  }

  // handle status leds state
  // blink();

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

void print_controller_state(ControllerState controllerState)
{
  Serial.println();
  // set bank indicator led
  Serial.print("Bank:\t\t");
  Serial.println(controllerState.bank);

  // set neopixel colors

  // backward-key
  Serial.print("Backward-key:\t");
  Serial.println(controllerState.backward);

  // forward-key
  Serial.print("Forward-key:\t");
  Serial.println(controllerState.forward);

  // fn-key
  Serial.print("Fn-key:\t\t");
  Serial.println(controllerState.fn);

  // 8 edit-keys
  Serial.print("Edit-keys:\t");
  for (uint8_t i = 0; i < 8; i++)
  {
    Serial.print(controllerState.edit[i]);
    Serial.print("  ");
  }
  Serial.println();

  // 16 media-keys
  Serial.print("Media-keys:\t");
  for (uint8_t i = 0; i < 16; i++)
  {
    Serial.print(controllerState.media[i]);
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