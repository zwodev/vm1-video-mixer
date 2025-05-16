#include <stdint.h>
#include <Adafruit_NeoPixel.h>
#include <stdio.h>
// #include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "stdlib.h"
#include "Keyboard.h"
// #include "RP2040-Encoder/quadrature.h"

// rotary encoder code taken from:
// https://www.reddit.com/r/raspberrypipico/comments/pacarb/sharing_some_c_code_to_read_a_rotary_encoder/

// four status leds pins
#define LED_PIN_01 0
#define LED_PIN_02 1
#define LED_PIN_03 2
#define LED_PIN_04 3
// input rows pins
#define ROW1 6
#define ROW2 5
#define ROW3 4
// output colums pins
#define COL1 15
#define COL2 14
#define COL3 13
#define COL4 12
#define COL5 11
#define COL6 10
#define COL7 9
#define COL8 8
#define COL9 7
// rotary encoder pins
#define ROTARY_PIN_A 26
#define ROTARY_PIN_B 27
// neopixel
#define NEOPIXELS_PIN 22
#define NEOPIXEL_COUNT 27
#define MESSAGEPACK_SIZE 27

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
// Quadrature_encoder<ROTARY_PIN_A, ROTARY_PIN_B> encoder = Quadrature_encoder<ROTARY_PIN_A, ROTARY_PIN_B>();
uint m_uPhasePosition = 0;
volatile int32_t encoder_value = 0;
int32_t encoder_value_old = 0;
int32_t m_iEncoderOffset;

// status leds
uint8_t status_leds[] = {LED_PIN_01, LED_PIN_02, LED_PIN_03, LED_PIN_04};
const uint8_t status_led_count = 4;

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

enum StatusLedState : uint8_t
{
  RUNNING,
  LED_0_ON,
  LED_1_ON,
  LED_2_ON,
  LED_3_ON,
};
StatusLedState led_state = RUNNING;

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
  ButtonState forward = ButtonState::NONE;
  ButtonState backward = ButtonState::NONE;
  ButtonState fn = ButtonState::NONE;
  ButtonState edit[8] = {ButtonState::NONE};
  ButtonState media[16] = {ButtonState::NONE};
};
#pragma pack()

// shared debug message between both cores
volatile char debug_msg[32] = "- no info -";
volatile bool debug_msg_ready = false;

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
    Serial.print(last_button);
    if (last_button_state == PRESSED)
    {
      Keyboard.press(last_button);
      Serial.print("Pressed\n");
    }
    else
    {
      Keyboard.release(last_button);
      Serial.print("Released\n");
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
  for (int8_t i = 0; i < status_led_count; i++)
  {
    digitalWrite(status_leds[i], i != led_index);
  }
}

void blink()
{
  unsigned long current_millis = millis();

  static int onboard_led_state = LOW;
  static long time = 0;
  static int8_t led_index = 0;

  if (current_millis - time > 250)
  {
    if (led_state == RUNNING)
    {
      // blink four status leds
      for (int8_t i = 0; i < status_led_count; i++)
      {
        digitalWrite(status_leds[i], i != led_index);
      }
      led_index--;
      if (led_index < 0)
        led_index = status_led_count - 1;
    }

    // blink on-board led
    if (onboard_led_state == HIGH)
    {
      onboard_led_state = LOW;
    }
    else
    {
      onboard_led_state = HIGH;
    }
    digitalWrite(LED_BUILTIN, onboard_led_state);

    // Serial.println(encoder.getCount());
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

const int8_t encoder_table[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0};

volatile uint8_t last_state = 0;

int8_t ReadEncoderDelta()
{
  uint8_t pin_a = gpio_get(ROTARY_PIN_A);
  uint8_t pin_b = gpio_get(ROTARY_PIN_B);
  uint8_t current_state = (pin_a << 1) | pin_b;

  uint8_t index = (last_state << 2) | current_state;
  int8_t delta = encoder_table[index];

  last_state = current_state;
  return delta;
}

void IRQCallback(uint gpio, uint32_t events)
{
  int8_t delta = ReadEncoderDelta();
  m_iEncoderOffset += delta;
}

void setup()
{
  Serial.begin(115200);

  // init leds
  pinMode(LED_BUILTIN, OUTPUT);
  for (uint8_t i = 0; i < status_led_count; i++)
  {
    pinMode(status_leds[i], OUTPUT);
  }

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
  // encoder.begin();
  // encoder.begin(pull_direction::up, resolution::quarter);
  gpio_init(ROTARY_PIN_A);
  gpio_set_dir(ROTARY_PIN_A, GPIO_IN);
  gpio_disable_pulls(ROTARY_PIN_A);

  gpio_init(ROTARY_PIN_B);
  gpio_set_dir(ROTARY_PIN_B, GPIO_IN);
  gpio_disable_pulls(ROTARY_PIN_B);

  gpio_set_irq_enabled_with_callback(ROTARY_PIN_A, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &IRQCallback);
  gpio_set_irq_enabled(ROTARY_PIN_B, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

  // neopixels
  strip.begin();
  strip.setBrightness(25);
  strip.show();

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

    // set neopixel colors
    int *color = colorForButtonState(controllerState.backward);
    strip.setPixelColor(0, colorFromArray(color));

    color = colorForButtonState(controllerState.forward);
    strip.setPixelColor(1, colorFromArray(color));

    color = colorForButtonState(controllerState.fn);
    strip.setPixelColor(18, colorFromArray(color));

    for (uint8_t i = 0; i < 8; i++)
    {
      color = colorForButtonState(controllerState.edit[i]);
      strip.setPixelColor(2 + i, colorFromArray(color));
    }

    for (uint8_t i = 0; i < 16; i++)
    {
      color = colorForButtonState(controllerState.media[i]);
      if (i < 8)
        strip.setPixelColor(17 - i, colorFromArray(color));
      else
        strip.setPixelColor(19 + i - 8, colorFromArray(color));
    }
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

void set_debug_msg_core1(const char *message)
{
  snprintf((char *)debug_msg, sizeof(debug_msg), "%s", message); // Safe string copy into the buffer
  debug_msg_ready = true;                                        // Set flag to indicate the message is ready
}

void setup1()
{
  set_debug_msg_core1("hello from core 1");
}

void loop1()
{
}