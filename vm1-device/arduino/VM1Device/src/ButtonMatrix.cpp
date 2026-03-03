#include <Arduino.h>
#include "hardware/gpio.h"
#include "pico/time.h"

#include "ButtonMatrix.h"
#include "DeviceBufferController.h"

const uint8_t NUM_ROWS = 3;
const uint8_t NUM_COLS = 9;
uint8_t rowPins[NUM_ROWS] = {ROW3, ROW2, ROW1};
uint8_t colPins[NUM_COLS] = {COL9, COL8, COL7, COL6, COL5, COL4, COL3, COL2, COL1};
bool currentButtonMatrix[NUM_ROWS][NUM_COLS] = {false};
bool previousButtonMatrix[NUM_ROWS][NUM_COLS] = {false};
ButtonEvent buttonEventMap[NUM_ROWS][NUM_COLS] = {
    {{EventType::NAVIGATION_BUTTON_EVENT, 0}, {EventType::EDIT_BUTTON_EVENT, 0}, {EventType::EDIT_BUTTON_EVENT, 1}, {EventType::EDIT_BUTTON_EVENT, 2}, {EventType::EDIT_BUTTON_EVENT, 3}, {EventType::EDIT_BUTTON_EVENT, 4}, {EventType::EDIT_BUTTON_EVENT, 5}, {EventType::EDIT_BUTTON_EVENT, 6}, {EventType::EDIT_BUTTON_EVENT, 7}},
    {{EventType::NAVIGATION_BUTTON_EVENT, 1}, {EventType::MEDIA_BUTTON_EVENT, 0}, {EventType::MEDIA_BUTTON_EVENT, 1}, {EventType::MEDIA_BUTTON_EVENT, 2}, {EventType::MEDIA_BUTTON_EVENT, 3}, {EventType::MEDIA_BUTTON_EVENT, 4}, {EventType::MEDIA_BUTTON_EVENT, 5}, {EventType::MEDIA_BUTTON_EVENT, 6}, {EventType::MEDIA_BUTTON_EVENT, 7}},
    {{EventType::NO_EVENT, 0},  {EventType::MEDIA_BUTTON_EVENT, 8}, {EventType::MEDIA_BUTTON_EVENT, 9}, {EventType::MEDIA_BUTTON_EVENT, 10}, {EventType::MEDIA_BUTTON_EVENT, 11}, {EventType::MEDIA_BUTTON_EVENT, 12}, {EventType::MEDIA_BUTTON_EVENT, 13}, {EventType::MEDIA_BUTTON_EVENT, 14}, {EventType::MEDIA_BUTTON_EVENT, 15}},
};
ButtonEvent lastButtonEvent = {EventType::NO_EVENT, 0};
bool last_button_state = RELEASED;

void initButtonMatrix()
{
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
}

void checkButtonMatrix()
{
    for (uint8_t i = 0; i < NUM_ROWS; i++)
    {
        for (uint8_t j = 0; j < NUM_COLS; j++)
        {
            if (previousButtonMatrix[i][j] != currentButtonMatrix[i][j])
            {
                lastButtonEvent = buttonEventMap[i][j];
                last_button_state = !currentButtonMatrix[i][j];
                return;
            }
        }
    }
    lastButtonEvent = {EventType::NO_EVENT, 0};
}

void updateButtonMatrix()
{
    for (uint8_t i = 0; i < NUM_COLS; i++)
    {
        gpio_put(colPins[i], true);
        sleep_us(10);

        uint32_t gpio_state = gpio_get_all();
        uint8_t row_values = (gpio_state >> 4) & 0b0111;

        currentButtonMatrix[2][i] = (row_values & 0b0001) == 0b0001;
        currentButtonMatrix[1][i] = (row_values & 0b0010) == 0b0010;
        currentButtonMatrix[0][i] = (row_values & 0b0100) == 0b0100;

        gpio_put(colPins[i], false);
        sleep_us(10);
    }
    deviceBuffer.fnPressed = currentButtonMatrix[2][0];
    
    checkButtonMatrix();

    if (lastButtonEvent.eventType != EventType::NO_EVENT)
    {
        if (last_button_state == PRESSED)
        {
            addButtonEventToBuffer(lastButtonEvent);
        }
    }

    // store current keyboard matrix
    for (uint8_t i = 0; i < NUM_ROWS; i++)
    {
        for (uint8_t j = 0; j < NUM_COLS; j++)
        {
            previousButtonMatrix[i][j] = currentButtonMatrix[i][j];
        }
    }
}


