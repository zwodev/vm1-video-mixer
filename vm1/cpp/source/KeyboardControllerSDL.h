#pragma once

#include "KeyboardController.h"

class KeyboardControllerSDL : public KeyboardController
{
public:
    KeyboardControllerSDL() = delete;
    explicit KeyboardControllerSDL(Registry &registry, EventBus &eventBus);
    ~KeyboardControllerSDL();

    void update(SDL_Event &event);

private:
    std::vector<SDL_Keycode> m_editKeys = {SDLK_Q, SDLK_W, SDLK_E, SDLK_R, SDLK_T, SDLK_Z, SDLK_U, SDLK_I};
    std::vector<SDL_Keycode> m_mediaKeys = {SDLK_A, SDLK_S, SDLK_D, SDLK_F, SDLK_G, SDLK_H, SDLK_J, SDLK_K,
                                                SDLK_Z, SDLK_X, SDLK_C, SDLK_V, SDLK_B, SDLK_N, SDLK_M, SDLK_COMMA};
};