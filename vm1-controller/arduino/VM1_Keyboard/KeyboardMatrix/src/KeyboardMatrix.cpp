#include <Arduino.h>
#include "hardware/gpio.h"
#include "pico/time.h"
#include "Keyboard.h"

#include "KeyboardMatrix.h"

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

void init_keyboard()
{
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

    Keyboard.begin();
}

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