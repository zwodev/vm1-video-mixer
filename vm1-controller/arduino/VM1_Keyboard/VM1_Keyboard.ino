#include <stdint.h>
#include "PluggableUSBHID.h"
#include "USBKeyboard.h"
#include "MatrixKeypad.h"
#include "pio_encoder.h"

// four status leds
#define LED_PIN_01 0
#define LED_PIN_02 1
#define LED_PIN_03 2
#define LED_PIN_04 3

// input rows
#define ROW1 6
#define ROW2 5
#define ROW3 4
// output colums
#define COL1 15
#define COL2 14
#define COL3 13
#define COL4 12
#define COL5 11
#define COL6 10
#define COL7 9
#define COL8 8
#define COL9 7
#define ROTARY_PIN_A 26
#define ROTARY_PIN_B 27

uint8_t status_leds[] = { LED_PIN_01, LED_PIN_02, LED_PIN_03, LED_PIN_04 };
const uint8_t status_led_count = 4;

const uint8_t rown = 3;                                                            //4 rows
const uint8_t coln = 9;                                                            //3 columns
uint8_t rowPins[rown] = { ROW3, ROW2, ROW1 };                                      //frist row is connect to pin 10, second to 9...
uint8_t colPins[coln] = { COL9, COL8, COL7, COL6, COL5, COL4, COL3, COL2, COL1 };  //frist column is connect to pin 6, second to 5...
char keymap[rown][coln] = {
  { '0', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',' },
  { '2', 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k' },
  { '1', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i' },
};


USBKeyboard Keyboard;
MatrixKeypad_t *keypad;  //keypad is the variable that you will need to pass to the other functions
char key;


PioEncoder encoder(ROTARY_PIN_A);
long encoder_value_old = 0;

char incoming_serial = 0;

enum STATUS_LED_STATE {
  RUNNING,
  LED_0_ON,
  LED_1_ON,
  LED_2_ON,
  LED_3_ON,
};

STATUS_LED_STATE led_state = RUNNING;

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  encoder.begin();
  keypad = MatrixKeypad_create((char *)keymap /* don't forget to do this cast */, rowPins, colPins, rown, coln);  //creates the keypad object
  for (uint8_t i = 0; i < status_led_count; i++) {
    pinMode(status_leds[i], OUTPUT);
  }
}


void loop() {
  // Scan Keyboard
  MatrixKeypad_scan(keypad);            //scans for a key press event
  if (MatrixKeypad_hasKey(keypad)) {    //if a key was pressed
    key = MatrixKeypad_getKey(keypad);  //get the key
    Serial.print(key);                  //prints the pressed key to the serial output
    if (key == '0') {
      Keyboard.key_code(RIGHT_ARROW);
    } else {
      Keyboard.printf("%c", key);
    }
  }

  // Rotary Encoder ==> UP/DOWN Keys
  long encoder_value = encoder.getCount();
  if (encoder_value > encoder_value_old) {
    Keyboard.key_code(UP_ARROW);
    encoder_value_old = encoder_value;
  } else if (encoder_value < encoder_value_old) {
    Keyboard.key_code(DOWN_ARROW);
    encoder_value_old = encoder_value;
  }

  // Handle Serial Input and StatusLEDs
  if (Serial.available() > 0) {
    incoming_serial = Serial.read();
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

  blink();
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
