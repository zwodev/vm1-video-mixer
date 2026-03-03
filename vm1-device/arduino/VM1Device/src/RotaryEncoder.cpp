#include <Arduino.h>
#include "hardware/gpio.h"
#include "pico/time.h"

#include "RotaryEncoder.h"
#include "DeviceBufferController.h"
#include "StateController.h"

#define DEBOUNCE_US 1000  // 1 ms debounce

static rotary_encoder_t* enc_a[32] = { 0 };

rotary_encoder_t encoder0;
rotary_encoder_t encoder1;
int32_t encoder0_position;
int32_t encoder1_position;

int encoder0_press_up = 0;
int encoder0_press_down = 0;
int encoder1_press_up = 0;
int encoder1_press_down = 0;


const int8_t transition_table[4][4] = {
  { 0, +1, -1, 0 },
  { -1, 0, 0, +1 },
  { +1, 0, 0, -1 },
  { 0, -1, +1, 0 },
};

void initRotaryEncoders(){
  encoder0 = {.pin_a = ROTARY_0_PIN_A, .pin_b = ROTARY_0_PIN_B};
  encoder1 = {.pin_a = ROTARY_1_PIN_A, .pin_b = ROTARY_1_PIN_B};
  rotary_encoder_init(&encoder0);
  rotary_encoder_init(&encoder1);

}

void updateRotaryEncoders() {
  int8_t enc0 = 0;
  if (enc0 = rotary_encoder_process(&encoder0, encoder0_position))
  {
    if (enc0 < 0)
    {
      encoder0_press_down = 0;
      encoder0_press_up++;
      if(encoder0_press_up % deviceState.rotarySensitivity == 0) { 
        addButtonEventToBuffer({EventType::ROTARY_EVENT, 0});
      }
    }
    else if (enc0 > 0)
    {
      encoder0_press_up = 0;
      encoder0_press_down++;
      if(encoder0_press_down % deviceState.rotarySensitivity == 0) {
        addButtonEventToBuffer({EventType::ROTARY_EVENT, 1});
      }
    }
  }
  
  int8_t enc1 = 0;
  if (enc1 = rotary_encoder_process(&encoder1, encoder1_position))
  {    
    if (enc1 < 0)
    {
      encoder1_press_down = 0;
      encoder1_press_up++;
      if(encoder1_press_up % deviceState.rotarySensitivity == 0) { 
        addButtonEventToBuffer({EventType::ROTARY_EVENT, 2});
      }
    }
    else if (enc1 > 0)
    {
      encoder1_press_up = 0;
      encoder1_press_down++;
      if(encoder1_press_down % deviceState.rotarySensitivity == 0) {
        addButtonEventToBuffer({EventType::ROTARY_EVENT, 3});
      }
    }
  }

}

void rotary_encoder_init(rotary_encoder_t* enc) {
  gpio_init(enc->pin_a);
  gpio_set_dir(enc->pin_a, GPIO_IN);
  gpio_pull_up(enc->pin_a);

  gpio_init(enc->pin_b);
  gpio_set_dir(enc->pin_b, GPIO_IN);
  gpio_pull_up(enc->pin_b);

  enc->position = 0;
  enc->last_change_us = time_us_32();

  enc_a[enc->pin_a] = enc;  // register this encoder by pin A
  enc_a[enc->pin_b] = enc;  // register this encoder by pin B

  enc->last_stable_state = (gpio_get(enc->pin_a) << 1) | gpio_get(enc->pin_b);

  gpio_set_irq_enabled_with_callback(
    enc->pin_a,
    GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
    true,
    &gpio_callback);

  gpio_set_irq_enabled(
    enc->pin_b,
    GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
    true);
}

int8_t rotary_encoder_process(rotary_encoder_t* enc, int32_t& position) {
  int8_t change = 0;
  if (!enc->state_changed_flag) return change;

  uint32_t now = time_us_32();
  if ((now - enc->last_change_us) >= DEBOUNCE_US) {
    // stable for at least debounce time
    if (enc->last_read_state != enc->last_stable_state) {
      int8_t movement = transition_table[enc->last_stable_state][enc->last_read_state];
      if (movement != 0) {
        enc->position += movement;
        position = enc->position;
        change = movement;
      }
      enc->last_stable_state = enc->last_read_state;
    }
    enc->state_changed_flag = false;
  }
  return change;
}

void gpio_callback(uint gpio, uint32_t events) {
  // get the correct rotary encoder from the static list:
  rotary_encoder_t* enc = enc_a[gpio];
  if (!enc) return;

  uint8_t a = gpio_get(enc->pin_a);
  uint8_t b = gpio_get(enc->pin_b);

  uint8_t current_state = (a << 1) | b;

  if (current_state != enc->last_read_state) {
    enc->last_read_state = current_state;
    enc->last_change_us = time_us_32();
    enc->state_changed_flag = true;
  }
}