#include "KeyboardController.h"

#include "stdio.h"

KeyboardController::KeyboardController()
{
}

KeyboardController::~KeyboardController()
{
}

void KeyboardController::update(SDL_Event &event)
{
    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        bool shiftPressed = event.key.mod & SDL_KMOD_SHIFT;
        switch (event.key.key)
        {
        case SDLK_UP:
            if (shiftPressed)
            {
            }
            else
            {
            }
            break;
        case SDLK_DOWN:
            if (shiftPressed)
            {
            }
            else
            {
            }
            break;
        case SDLK_LEFT:
            break;
        case SDLK_RIGHT:
            break;
        default:
            break;
        }

        for(int i = 0; i < m_editKeys.size(); ++i) 
        {

        }

        for(int i = 0; i < m_mediaKeys.size(); ++i) 
        {
            // event = bank * i
        }
    }
}

char KeyboardController::getKeyPress()
{
}
