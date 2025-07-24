#include "KeyboardControllerSDL.h"

#include "stdio.h"

KeyboardControllerSDL::KeyboardControllerSDL(Registry& registry, EventBus& eventBus) :
    KeyboardController(registry, eventBus)
{
}

KeyboardControllerSDL::~KeyboardControllerSDL()
{
}

void KeyboardControllerSDL::update(SDL_Event& event)
{
    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        bool shiftPressed = event.key.mod & SDL_KMOD_SHIFT;
        switch (event.key.key)
        {
        case SDLK_UP:
            if (shiftPressed)
            {
                m_eventBus.publish(NavigationEvent(NavigationEvent::Type::IncreaseValue));
            }
            else
            {
                m_eventBus.publish(NavigationEvent(NavigationEvent::Type::FocusPrevious));
            }
            return;
            break;
        case SDLK_DOWN:
            if (shiftPressed)
            {
                m_eventBus.publish(NavigationEvent(NavigationEvent::Type::DecreaseValue));
            }
            else
            {
                m_eventBus.publish(NavigationEvent(NavigationEvent::Type::FocusNext));
            }
            return;
            break;
        case SDLK_LEFT:
            if (shiftPressed)
            {
                m_eventBus.publish(NavigationEvent(NavigationEvent::Type::BankUp));
            }
            else 
            {
                m_eventBus.publish(NavigationEvent(NavigationEvent::Type::HierarchyUp));
            }
            return;
            break;
        case SDLK_RIGHT:
            if (shiftPressed)
            {
                m_eventBus.publish(NavigationEvent(NavigationEvent::Type::BankDown));
            }
            else
            {
                m_eventBus.publish(NavigationEvent(NavigationEvent::Type::HierarchyDown));
                m_eventBus.publish(NavigationEvent(NavigationEvent::Type::SelectItem));
            }
            return;
            break;
        default:
            break;
        }

        for(int i = 0; i < m_editKeys.size(); ++i) 
        {
            if (event.key.key == m_editKeys[i]) {
                m_eventBus.publish(EditModeEvent(i));
                return;
            }
        }

        for(int i = 0; i < m_mediaKeys.size(); ++i) 
        {
            int mediaSlotId = (m_registry.inputMappings().bank * 16) + i;
            if (event.key.key == m_mediaKeys[i]) {
                m_eventBus.publish(MediaSlotEvent(mediaSlotId));
                return;
            }
        }
    }
}