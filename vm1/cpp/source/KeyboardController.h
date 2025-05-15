
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

MSGPACK_ADD_ENUM(ButtonState);

struct ControllerState {
    ButtonState forward;
    ButtonState backward;
    ButtonState fn;
    ButtonState edit[8];
    ButtonState media[16];

    MSGPACK_DEFINE(forward, backward, fn, edit, media);
};

class KeyboardController {
    
public:
    KeyboardController() = default;
    ~KeyboardController() = default;

    bool connect(const std::string& port);
    void disconnect();
    void update(const ControllerState& state);

private:
    int m_fd = -1;
};