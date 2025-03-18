#include <stdint.h>
#include "Keyboard.h"
#include "pio_encoder.h"

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
uint8_t rowPins[NUM_ROWS] = { ROW3, ROW2, ROW1 };
uint8_t colPins[NUM_COLS] = { COL9, COL8, COL7, COL6, COL5, COL4, COL3, COL2, COL1 };
bool current_keyboard_matrix[NUM_ROWS][NUM_COLS] = { false };
bool previous_keyboard_matrix[NUM_ROWS][NUM_COLS] = { false };
char keymap[NUM_ROWS][NUM_COLS] = {
  { KEY_LEFT_ARROW, 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',' },
  { KEY_RIGHT_ARROW, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k' },
  { KEY_LEFT_SHIFT, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i' },
};
char last_button = '\0';
bool last_button_state = RELEASED;


// encoder
PioEncoder encoder(ROTARY_PIN_A);
long encoder_value_old = 0;

// status leds
uint8_t status_leds[] = { LED_PIN_01, LED_PIN_02, LED_PIN_03, LED_PIN_04 };
const uint8_t status_led_count = 4;

enum STATUS_LED_STATE {
  RUNNING,
  LED_0_ON,
  LED_1_ON,
  LED_2_ON,
  LED_3_ON,
};
STATUS_LED_STATE led_state = RUNNING;

void check_keyboard_matrix() {
  for (uint8_t i = 0; i < NUM_ROWS; i++) {
    for (uint8_t j = 0; j < NUM_COLS; j++) {
      if (previous_keyboard_matrix[i][j] != current_keyboard_matrix[i][j]) {
        last_button = keymap[i][j];
        last_button_state = !current_keyboard_matrix[i][j];
        return;
      }
    }
  }
  last_button = '\0';
}

void update_keyboard() {
  for (uint8_t i = 0; i < NUM_COLS; i++) {
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

  if (last_button != '\0') {
    Serial.print(last_button);
    if (last_button_state == PRESSED) {
      Serial.print("Pressed\n");
    } else {
      Serial.print("Released\n");
    }
  }

  // store current keyboard matrix
  for (uint8_t i = 0; i < NUM_ROWS; i++) {
    for (uint8_t j = 0; j < NUM_COLS; j++) {
      previous_keyboard_matrix[i][j] = current_keyboard_matrix[i][j];
    }
  }
}

void setup() {
  Serial.begin(115200);

  // init encoder
  encoder.begin();

  // init leds
  pinMode(LED_BUILTIN, OUTPUT);
  for (uint8_t i = 0; i < status_led_count; i++) {
    pinMode(status_leds[i], OUTPUT);
  }

  // init button-matrix
  for (uint8_t i = 0; i < NUM_ROWS; i++) {
    gpio_init(rowPins[i]);
    gpio_set_dir(rowPins[i], GPIO_IN);
  }
  for (uint8_t i = 0; i < NUM_COLS; i++) {
    gpio_init(colPins[i]);
    gpio_set_dir(colPins[i], GPIO_OUT);
    gpio_put(colPins[i], false);
  }

  // hid-keyboard
  Keyboard.begin();

#ifdef DEBUG
  d_current_micros = 0;
  d_previous_micros = 0;
#endif
}

void loop() {
#ifdef DEBUG
  d_current_micros = micros();
#endif
  // KEY_LEFT_SHIFT
  // KEY_TAB
  // KEY_UP_ARROW
  // KEY_DOWN_ARROW
  // KEY_LEFT_ARROW
  // KEY_RIGHT_ARROW

  // Scan Keyboard
  // Keyboard.press('a');
  // delay(1000);
  // Keyboard.release('a');
  // delay(1000);
  update_keyboard();


  // Rotary Encoder ==> UP/DOWN Keys
  long encoder_value = encoder.getCount();
  if (encoder_value > encoder_value_old) {
    Keyboard.press(KEY_UP_ARROW);
    delay(1);
    Keyboard.release(KEY_UP_ARROW);
    encoder_value_old = encoder_value;
  } else if (encoder_value < encoder_value_old) {
    Keyboard.press(KEY_DOWN_ARROW);
    delay(1);
    Keyboard.release(KEY_DOWN_ARROW);
    encoder_value_old = encoder_value;
  }

  // handle serial input and status leds
  if (Serial.available() > 0) {
    char incoming_serial = Serial.read();
    Serial.println(incoming_serial);
    switch (incoming_serial) {
      case '0':
        Serial.println("Running");
        led_state = RUNNING;
        break;
      case '1':
        Serial.println("LED 0");
        led_state = LED_0_ON;
        set_status_led(0);
        break;
      case '2':
        Serial.println("LED 1");
        led_state = LED_1_ON;
        set_status_led(1);
        break;
      case '3':
        Serial.println("LED 2");
        led_state = LED_2_ON;
        set_status_led(2);
        break;
      case '4':
        Serial.println("LED 3");
        led_state = LED_3_ON;
        set_status_led(3);
        break;
      default:
        break;
    }
  }

  // handle status leds state
  blink();

#ifdef DEBUG
  uint32_t d_time_after_main_loop = micros();
  d_main_loop_duration_micros = d_time_after_main_loop - d_current_micros;
  // check how long the main loop takes
  // (hint/todo: it actually should average the time of all the
  //             iterations it does up to the next debug output)
  if (d_current_micros - d_previous_micros > d_debug_log_interval_micros) {
    Serial.print("Main loop duration in us:");
    Serial.println(d_main_loop_duration_micros);
    d_previous_micros = d_current_micros;
  }
#endif
}

void set_status_led(uint8_t led_index) {
  for (int8_t i = 0; i < status_led_count; i++) {
    digitalWrite(status_leds[i], i != led_index);
  }
}

void blink() {
  unsigned long current_millis = millis();

  static int onboard_led_state = LOW;
  static long time = 0;
  static int8_t led_index = 0;

  if (current_millis - time > 250) {
    if (led_state == RUNNING) {
      // blink four status leds
      for (int8_t i = 0; i < status_led_count; i++) {
        digitalWrite(status_leds[i], i != led_index);
      }
      led_index--;
      if (led_index < 0) led_index = status_led_count - 1;
    }

    // blink on-board led
    if (onboard_led_state == HIGH) {
      onboard_led_state = LOW;
    } else {
      onboard_led_state = HIGH;
    }
    digitalWrite(LED_BUILTIN, onboard_led_state);

    // Serial.println(encoder.getCount());
    time = current_millis;
  }
}
