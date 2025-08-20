#pragma once

#include "KeyboardController.h"

class KeyboardControllerSDL : public KeyboardController
{
public:
    KeyboardControllerSDL() = delete;
    explicit KeyboardControllerSDL(Registry &registry, EventBus &eventBus);
    ~KeyboardControllerSDL();

    void update(SDL_Event& event);

private:
    std::vector<SDL_Keycode> m_editKeys = {SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7, SDLK_8};
    std::vector<SDL_Keycode> m_mediaKeys = {SDLK_Q, SDLK_W, SDLK_E, SDLK_R, SDLK_T, SDLK_Y, SDLK_U, SDLK_I,
                                                SDLK_A, SDLK_S, SDLK_D, SDLK_F, SDLK_G, SDLK_H, SDLK_J, SDLK_K};
};