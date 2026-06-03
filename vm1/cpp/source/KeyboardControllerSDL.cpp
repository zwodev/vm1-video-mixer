/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

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
        bool isShiftPressed = event.key.mod & SDL_KMOD_SHIFT;
        if(!isShiftPressed)  // if SHIFT not pressed
        {
            switch (event.key.key)
            {
                case SDLK_RIGHT:
                case SDLK_RETURN:
                    m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationEnter));
                    return;
                    break;
                case SDLK_LEFT:
                case SDLK_ESCAPE:
                    m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationExit));
                    return;
                    break;
                case SDLK_UP:
                    m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationUp));
                    return;
                    break;
                case SDLK_DOWN:
                    m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationDown));
                    return;
                    break;
                case SDLK_PAGEUP:
                case SDLK_PERIOD:
                    m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationAuxDown));
                    return;
                    break;
                case SDLK_PAGEDOWN:
                case SDLK_COMMA:
                    m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationAuxUp));
                    return;
                    break;
                // case SDLK_SPACE:
                //     m_eventBus.publish(NavigationEvent(NavigationEvent::Type::Screenshot));
                //     return;
                //     break;
                default:
                    break;
            }
        } 
        else  // if SHIFT pressed
        {
            switch (event.key.key)
            {
                case SDLK_RIGHT:
                case SDLK_RETURN:
                    m_eventBus.publish(ValueChangeEvent(ValueChangeEvent::Type::Up, 0));
                    return;
                    break;
                case SDLK_LEFT:
                    m_eventBus.publish(ValueChangeEvent(ValueChangeEvent::Type::Down, 0));
                    return;
                    break;
                case SDLK_UP:
                    m_eventBus.publish(ValueChangeEvent(ValueChangeEvent::Type::Up, 1));
                    return;
                    break;
                case SDLK_DOWN:
                    m_eventBus.publish(ValueChangeEvent(ValueChangeEvent::Type::Down, 1));
                    return;
                    break;
                // case SDLK_PAGEUP:
                //     m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationAuxDown));
                //     return;
                //     break;
                // case SDLK_PAGEDOWN:
                //     m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationAuxUp));
                //     return;
                //     break;
                // case SDLK_SPACE:
                //     m_eventBus.publish(NavigationEvent(NavigationEvent::Type::Screenshot));
                //     return;
                //     break;
                default:
                    break;
            }
        }
        // switch (event.key.key)
        // {
        // case SDLK_ESCAPE:
        //     m_eventBus.publish(SystemEvent(SystemEvent::Type::Exit));
        //     return;
        //     break;
        // case SDLK_UP:
        //     if (isShiftPressed)
        //     {
        //         m_eventBus.publish(ValueChangeEvent(ValueChangeEvent::Type::Up, 1));
        //     }
        //     else
        //     {
        //         m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationUp));
        //     }
        //     return;
        //     break;
        // case SDLK_DOWN:
        //     if (isShiftPressed)
        //     {
        //         m_eventBus.publish(ValueChangeEvent(ValueChangeEvent::Type::Down, 1));
        //     }
        //     else
        //     {
        //         m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationDown));
        //     }
        //     return;
        //     break;
        // case SDLK_LEFT:
        //     if (isShiftPressed)
        //     {
        //         // TODO: How to deal with FnNavigation with keyboard control?
        //         m_eventBus.publish(ValueChangeEvent(ValueChangeEvent::Type::Down, 0));
        //         //m_eventBus.publish(NavigationEvent(NavigationEvent::Type::FnNavigationLeft));
        //     }
        //     else
        //     {
        //         m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationLeft));
        //     }
        //     return;
        //     break;
        // case SDLK_RIGHT:
        //     if (isShiftPressed)
        //     {
        //         m_eventBus.publish(ValueChangeEvent(ValueChangeEvent::Type::Up, 0));
        //         //m_eventBus.publish(NavigationEvent(NavigationEvent::Type::FnNavigationRight));
        //     }
        //     else
        //     {
        //         m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationRight));
        //     }
        //     return;
        //     break;
        // case SDLK_PAGEUP:
        //     m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationAuxDown));
        //     return;
        //     break;
        // case SDLK_PAGEDOWN:
        //     m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationAuxUp));
        //     return;
        //     break;
        // case SDLK_SPACE:
        //     m_eventBus.publish(NavigationEvent(NavigationEvent::Type::Screenshot));
        //     return;
        //     break;
        // default:
        //     break;
        // }

        for(size_t i = 0; i < m_editKeys.size(); ++i) 
        {
            if (event.key.key == m_editKeys[i]) {
                if(isShiftPressed){
                    m_eventBus.publish(BankChangeEvent(int(i)));
                }
                else {
                    m_eventBus.publish(EditModeEvent(int(i)));
                }                
                return;
            }
        }
        if (event.key.scancode == SDL_SCANCODE_GRAVE) {
            m_eventBus.publish(EditModeEvent(0));
        }
        for(size_t i = 0; i < m_mediaKeys.size(); ++i) 
        {
            int mediaSlotId = (m_registry.inputMappings().focusedBank * MEDIA_BUTTON_COUNT) + int(i);
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