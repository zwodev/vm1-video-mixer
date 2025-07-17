#pragma once

#include <vector>
#include <SDL3/SDL.h>
#include "EventBus.h"

class KeyboardController
{
private:
    std::vector<SDL_Keycode> m_editKeys = {SDLK_Q, SDLK_W, SDLK_E, SDLK_R, SDLK_T, SDLK_Z, SDLK_U, SDLK_I};
    std::vector<SDL_Keycode> m_mediaKeys = {SDLK_A, SDLK_S, SDLK_D, SDLK_F, SDLK_G, SDLK_H, SDLK_J, SDLK_K,
                                                SDLK_Z, SDLK_X, SDLK_C, SDLK_V, SDLK_B, SDLK_N, SDLK_M, SDLK_COMMA};
    EventBus &eventBus;
    
    KeyboardController();

public:
    KeyboardController(EventBus &eventBus);
    ~KeyboardController();
    void update(SDL_Event &event);
    char getKeyPress();
};