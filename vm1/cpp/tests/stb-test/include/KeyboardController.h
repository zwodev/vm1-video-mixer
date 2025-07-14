#pragma once

/**
 * @class KeyboardController
 * @brief Handles keyboard input and manages terminal raw mode.
 *
 * The KeyboardController class provides functionality to enable or disable
 * raw mode for keyboard input and to capture key presses from the user.
 */
class KeyboardController
{
private:
    void setRawMode(bool enable);

public:
    KeyboardController();
    ~KeyboardController();
    char getKeyPress();
};