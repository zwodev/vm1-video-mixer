#pragma once

#include "KeyboardController.h"

#include <linux/input.h>

class KeyboardControllerLinux : public KeyboardController
{
public:
    KeyboardControllerLinux() = delete;
    explicit KeyboardControllerLinux(Registry& registry, EventBus& eventBus);
    ~KeyboardControllerLinux();

    void update(input_event &event);

private:
    bool m_isShiftPressed = false;
    std::vector<__u16> m_editKeys = {KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8};
    std::vector<__u16> m_mediaKeys = {KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I,
                                        KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K};
};