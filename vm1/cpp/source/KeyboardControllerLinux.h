#pragma once

#include "KeyboardController.h"

#include <linux/input.h>

class KeyboardControllerLinux : public KeyboardController
{
public:
    KeyboardControllerLinux() = delete;
    explicit KeyboardControllerLinux(Registry &registry, EventBus &eventBus);
    ~KeyboardControllerLinux();

    void update(input_event &event);

private:
    bool m_isShiftPressed = false;
    std::vector<__u16> m_editKeys = {KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Z, KEY_U, KEY_I};
    std::vector<__u16> m_mediaKeys = {KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K,
                                                KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_COMMA};
};