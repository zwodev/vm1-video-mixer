// una biblioteca por un codificador rotatorio de julián

#include "pico/stdlib.h"

#include "rotatorio.h"

#define PIN_ENCODER_1_A 16
#define PIN_ENCODER_1_B 17
#define PIN_ENCODER_0_A 18
#define PIN_ENCODER_0_B 19


rotary_encoder_t encoder0 = { .pin_a = PIN_ENCODER_0_A, .pin_b = PIN_ENCODER_0_B };
rotary_encoder_t encoder1 = { .pin_a = PIN_ENCODER_1_A, .pin_b = PIN_ENCODER_1_B };
int32_t encoder0_position;
int32_t encoder1_position;

void setup() {
  Serial.begin(115200);
  while (!Serial) {};

  rotary_encoder_init(&encoder0);
  rotary_encoder_init(&encoder1);
}

void loop() {

  int8_t enc0 = 0;
  if (enc0 = rotary_encoder_process(&encoder0, encoder0_position)) {
    String res = "Encoder 0: ";
    res += encoder0_position;
    if (enc0 < 0) {
      res += ", down";
    } else if (enc0 > 0) {
      res += ", up";
    }
    Serial.println(res);
  }


  int8_t enc1 = 0;
  if (enc1 = rotary_encoder_process(&encoder1, encoder1_position)) {
    String res = "Encoder 1: ";
    res += encoder1_position;
    if (enc1 < 0) {
      res += ", down";
    } else if (enc1 > 0) {
      res += ", up";
    }
    Serial.println(res);
  }

}