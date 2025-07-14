#ifndef ROTATORIO_H
#define ROTATORIO_H

#include <stdint.h>
#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct {
    uint8_t pin_a;
    uint8_t pin_b;
    int32_t position;
    volatile uint8_t last_stable_state;  // last debounced stable state
    volatile uint8_t last_read_state;    // last raw read state
    volatile uint32_t last_change_us;    // last time state changed (micros)
    volatile bool state_changed_flag;    // set by ISR, processed outside

  } rotary_encoder_t;

  void rotary_encoder_init(rotary_encoder_t* enc);
  int8_t rotary_encoder_process(rotary_encoder_t* enc, int32_t& position);
  void gpio_callback(uint gpio, uint32_t events);

#ifdef __cplusplus
}
#endif

#endif