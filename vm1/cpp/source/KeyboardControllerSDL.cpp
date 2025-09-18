#include "KeyboardControllerSDL.h"
#include "VM1DeviceDefinitions.h"
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
        m_eventBus.publish(SystemEvent(SystemEvent::Type::KeyDown));


        bool isShiftPressed = event.key.mod & SDL_KMOD_SHIFT;
        switch (event.key.key)
        {
        case SDLK_ESCAPE:
            m_eventBus.publish(SystemEvent(SystemEvent::Type::Exit));
            return;
            break;
        case SDLK_UP:
            if (isShiftPressed)
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
            if (isShiftPressed)
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
            if (isShiftPressed)
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
            if (isShiftPressed)
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
            int mediaSlotId = (m_registry.inputMappings().bank * MEDIA_BUTTON_COUNT) + i;
            if (event.key.key == m_mediaKeys[i]) {
                if(isShiftPressed){
                    m_eventBus.publish(MediaSlotEvent(mediaSlotId, false)); // do not trigger playback
                }
                else {
                    m_eventBus.publish(MediaSlotEvent(mediaSlotId)); 
                }
                return;
            }
        }
    }
}