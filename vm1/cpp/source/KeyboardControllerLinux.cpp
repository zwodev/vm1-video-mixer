#include "KeyboardControllerLinux.h"
#include "VM1DeviceDefinitions.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

KeyboardControllerLinux::KeyboardControllerLinux(Registry& registry, EventBus& eventBus) : 
    KeyboardController(registry, eventBus)
{
}

KeyboardControllerLinux::~KeyboardControllerLinux()
{
}


void KeyboardControllerLinux::update(input_event& event)
{   
    if (event.value == 0) {
        switch (event.code)
        {
            case KEY_LEFTSHIFT:
                m_isShiftPressed = false;
                return;
                break;
            default:
                break;
        }
    }
    else if (event.value == 1)
    {
        switch (event.code)
        {
            case KEY_ESC:
                m_eventBus.publish(SystemEvent(SystemEvent::Type::Exit));
                return;
                break;
            case KEY_LEFTSHIFT:
                m_isShiftPressed = true;
                return;
                break;
            case KEY_UP:
                if (m_isShiftPressed)
                {
                    m_eventBus.publish(ValueChangeEvent(ValueChangeEvent::Type::Up, 1));
                }
                else
                {
                    m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationUp));
                }
                return;
                break;
            case KEY_DOWN:
                if (m_isShiftPressed)
                {
                    m_eventBus.publish(ValueChangeEvent(ValueChangeEvent::Type::Down, 1));
                }
                else
                {
                    m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationDown));
                }
                return;
                break;
            case KEY_LEFT:
                if (m_isShiftPressed)
                {
                    m_eventBus.publish(ValueChangeEvent(ValueChangeEvent::Type::Down, 0));
                }
                else 
                {
                    m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationLeft));
                }
                return;
                break;
            case KEY_RIGHT:
                if (m_isShiftPressed)
                {
                    m_eventBus.publish(ValueChangeEvent(ValueChangeEvent::Type::Up, 0));
                }
                else 
                {
                    m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationRight));
                }
                return;
                break;
        case KEY_PAGEUP:
            m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationAuxDown));
            return;
            break;
        case KEY_PAGEDOWN:
            m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationAuxUp));
            return;
            break;
            default:
                break;
        case KEY_SPACE:
            m_eventBus.publish(NavigationEvent(NavigationEvent::Type::Screenshot));
            return;
            break;
        }

        for(size_t i = 0; i < m_editKeys.size(); ++i) 
        {
            if (event.code == m_editKeys[i]) {
                if(m_isShiftPressed){
                    m_eventBus.publish(BankChangeEvent(int(i)));
                }
                else {
                    m_eventBus.publish(EditModeEvent(int(i)));
                }                
                return;
            }
        }

        for(size_t i = 0; i < m_mediaKeys.size(); ++i) 
        {
            int mediaSlotId = (m_registry.inputMappings().bank * MEDIA_BUTTON_COUNT) + int(i);
            if (event.code == m_mediaKeys[i]) {
                if(m_isShiftPressed){
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