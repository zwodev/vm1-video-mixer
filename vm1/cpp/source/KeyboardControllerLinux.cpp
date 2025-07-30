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
    bool isShiftPressed = false;

    if (event.value == 0) {
        switch (event.code)
        {
            case KEY_LEFTSHIFT:
                isShiftPressed = false;
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
            case KEY_LEFTSHIFT:
                isShiftPressed = true;
                return;
                break;
            case KEY_UP:
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
            case KEY_DOWN:
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
            case KEY_LEFT:
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
            case KEY_RIGHT:
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
            if (event.code == m_editKeys[i]) {
                m_eventBus.publish(EditModeEvent(i));
                return;
            }
        }

        for(int i = 0; i < m_mediaKeys.size(); ++i) 
        {
            int mediaSlotId = (m_registry.inputMappings().bank * MEDIA_BUTTON_COUNT) + i;
            if (event.code == m_mediaKeys[i]) {
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