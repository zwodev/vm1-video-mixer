
#pragma once

#include <msgpack.hpp>

enum ButtonState
{
    NONE,
    EMPTY,
    FILE_ASSET,
    LIVECAM,
    SHADER,
    FILE_ASSET_ACTIVE,
    LIVECAM_ACTIVE,
    SHADER_ACTIVE,
};

struct ControllerState {
    ButtonState forward;
    ButtonState backward;
    ButtonState fn;
    ButtonState edit[8];
    ButtonState media[16];

    MSGPACK_DEFINE(forward, backward, fn, edit, media);
}

class KeyboardController {
    KeyboardController() = default;
    ~KeyboardController() = default;
};