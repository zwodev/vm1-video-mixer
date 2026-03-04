#include "DeviceController.h"
#include "VM1DeviceDefinitions.h"
#include <string>
#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <lgpio.h>

DeviceController::DeviceController(EventBus& eventBus, Registry& registry) : 
    m_eventBus(eventBus),
    m_registry(registry)
{}

DeviceController::~DeviceController(){
    disconnect();
}

// Helper: Set up serial port
bool DeviceController::connect(const std::string& port)
{
    bool isProVersion = false;

    std::cout << "Failed to open Serial Connection, trying I2C..." << std::endl;
    if(connectI2C()) 
    {
        std::cout << "Connected to VM1-Device via I2C." << std::endl;
        isProVersion = true;
    } 
    else
    {
        std::cout << "Failed to open I2C, using standard keyboard mode." << std::endl;
        return false;
    }    

    m_registry.settings().isProVersion = isProVersion;
    return true;
}


bool DeviceController::connectI2C()
{
    m_gpioHandler = lgGpiochipOpen(0); // Usually GPIO chip 0 is correct
    if (m_gpioHandler < 0)
    {
        std::cerr << "Failed to open gpiochip\n";
        return false;
    }
    
    m_i2c_handle = lgI2cOpen(1, 0x08, 0); // bus 1, address 0x08
    if (m_i2c_handle < 0)
    {
        std::cerr << "Failed to open I2C device\n";
        return false;
    }
    return true;
}

void DeviceController::disconnect()
{


    if(m_i2c_handle >= 0)
    {
        lgI2cClose(m_i2c_handle);
        m_i2c_handle = -1;
        
    }

    if (m_gpioHandler >= 0) {
        lgGpiochipClose(m_gpioHandler);
        m_gpioHandler = -1;
    }
}
     

void DeviceController::send(const VM1DeviceState& vm1DeviceState)
{
    // Do not send state when nothing has changed (aka hash is the same)
    size_t hash = vm1DeviceState.hash();
    if (hash == m_lastHash)
        return;

    m_lastHash = hash;

    if (m_i2c_handle >= 0) 
    {
        lgI2cWriteDevice(m_i2c_handle, reinterpret_cast<const char*>(&vm1DeviceState), sizeof(VM1DeviceState));
    }

}

void DeviceController::requestVM1DeviceBuffer() 
{
    if(m_i2c_handle < 0) return;
    
    DeviceBuffer deviceBuffer;
    int count = lgI2cReadDevice(m_i2c_handle, reinterpret_cast<char*>(&deviceBuffer), sizeof(DeviceBuffer));
    if (count != sizeof(DeviceBuffer))
    {
        std::cerr << "Failed to read from I2C device\n";
    }
    else
    {
        // if (deviceBuffer.buttonEvents->eventType != NO_EVENT && deviceBuffer.fnPressed) {
        //     printf("FN + ");
        // }
        // if (deviceBuffer.buttonEvents->eventType == EDIT_BUTTON_EVENT) {
        //     printf("EDIT BUTTON #%d\n", deviceBuffer.buttonEvents[0].buttonId);
        // }
        // else if (deviceBuffer.buttonEvents->eventType == MEDIA_BUTTON_EVENT) {
        //     printf("MEDIA BUTTON #%d\n", deviceBuffer.buttonEvents[0].buttonId);
        // }
        // else if (deviceBuffer.buttonEvents->eventType == NAVIGATION_BUTTON_EVENT) {
        //     printf("NAVIGATION #%d\n", deviceBuffer.buttonEvents[0].buttonId);
        // }
        // else if (deviceBuffer.buttonEvents->eventType == ROTARY_EVENT) {
        //     printf("ROTARY #%d\n", deviceBuffer.buttonEvents[0].buttonId);
        // }

        bool isFnPressed = deviceBuffer.fnPressed;
        int buttonId = -1;

        for(uint8_t i = 0; i < sizeof(deviceBuffer.buttonEvents)/sizeof(ButtonEvent); i++)
        {
            buttonId = deviceBuffer.buttonEvents[i].buttonId;

            switch(deviceBuffer.buttonEvents[i].eventType) 
            {
                case EDIT_BUTTON_EVENT:
                    isFnPressed ? m_eventBus.publish(BankChangeEvent(buttonId))
                                : m_eventBus.publish(EditModeEvent(buttonId));
                    break;
                case MEDIA_BUTTON_EVENT: {
                    int mediaSlotId = (m_registry.inputMappings().bank * MEDIA_BUTTON_COUNT) + buttonId;
                    m_eventBus.publish(MediaSlotEvent(mediaSlotId, !isFnPressed));
                    break; }
                case NAVIGATION_BUTTON_EVENT:
                    if(buttonId == NAVIGATION_BUTTON_LEFT)
                    {
                        m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationLeft));
                    }
                    else if (buttonId == NAVIGATION_BUTTON_RIGHT)
                    {
                        m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationRight));
                    }
                    else 
                    {
                        printf("Unknown Navigation Button Id #%d\n", buttonId);
                    }
                    break;
                case ROTARY_EVENT:
                    if (buttonId == PRIMARY_ENCODER_CCW)
                    {
                        isFnPressed ? m_eventBus.publish(ValueChangeEvent(ValueChangeEvent::Type::Down, 0)) 
                                    : m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationUp));
                    }
                    else if (buttonId == PRIMARY_ENCODER_CW)
                    {
                        isFnPressed ? m_eventBus.publish(ValueChangeEvent(ValueChangeEvent::Type::Up, 0)) 
                                    : m_eventBus.publish(NavigationEvent(NavigationEvent::Type::NavigationDown));
                    }
                    else if (buttonId == SECONDARY_ENCODER_CCW)
                    {
                        isFnPressed ? m_eventBus.publish(ValueChangeEvent(ValueChangeEvent::Type::Down, 1)) 
                                    : (void)printf("secondary rotary down, not implemented\n");
                    }
                    else if (buttonId == SECONDARY_ENCODER_CW)
                    {
                        isFnPressed ? m_eventBus.publish(ValueChangeEvent(ValueChangeEvent::Type::Up, 1)) 
                                    : (void)printf("secondary rotary up, not implemented\n");
                    }
                    else
                    {
                        printf("Unknown Rotary Button Id #%d\n", buttonId);
                    }
                    break;
                default:
                
                    break;
            }
        }
        if (deviceBuffer.buttonEvents->eventType != NO_EVENT)
            m_eventBus.publish(SystemEvent(SystemEvent::KeyDown)); // used for time-out in kiosk mode

        /*
        for(uint8_t i = 0; i < sizeof(deviceBuffer.buttons); ++i) 
        {
            char currentChar = deviceBuffer.buttons[i];
            bool isShiftPressed = deviceBuffer.shiftPressed;

            if(currentChar == '\0') 
                break;

            switch (currentChar)
            {
                case 218: // KEY_UP:
                    if (isShiftPressed)
                    {
                        m_eventBus.publish(NavigationEvent(NavigationEvent::Type::DecreaseValue));
                    }
                    else
                    {
                        m_eventBus.publish(NavigationEvent(NavigationEvent::Type::FocusPrevious));
                    }
                    return;
                    break;
                case 217: // KEY_DOWN:
                    if (isShiftPressed)
                    {
                        m_eventBus.publish(NavigationEvent(NavigationEvent::Type::IncreaseValue));
                    }
                    else
                    {
                        m_eventBus.publish(NavigationEvent(NavigationEvent::Type::FocusNext));
                    }
                    return;
                    break;
                case 216: // KEY_LEFT
                    if (isShiftPressed)
                    {
                        m_eventBus.publish(NavigationEvent(NavigationEvent::Type::BankUp));
                        m_eventBus.publish(SystemEvent(SystemEvent::KeyDown));
                    }
                    else 
                    {
                        m_eventBus.publish(NavigationEvent(NavigationEvent::Type::HierarchyUp));
                    }
                    return;
                    break;
                case 215: // KEY_RIGHT
                    if (isShiftPressed)
                    {
                        m_eventBus.publish(NavigationEvent(NavigationEvent::Type::BankDown));
                        m_eventBus.publish(SystemEvent(SystemEvent::KeyDown));
                    }
                    else
                    {
                        m_eventBus.publish(NavigationEvent(NavigationEvent::Type::HierarchyDown));
                        m_eventBus.publish(NavigationEvent(NavigationEvent::Type::SelectItem));
                    }
                    return;
                    break;
                case 43: // + (plus)
                    if (isShiftPressed)
                    {
                        m_eventBus.publish(NavigationEvent(NavigationEvent::Type::DecreaseAuxValue));
                        m_eventBus.publish(SystemEvent(SystemEvent::KeyDown));
                    }
                    // else
                    // {
                    //     m_eventBus.publish(NavigationEvent(NavigationEvent::Type::HierarchyDown));
                    //     m_eventBus.publish(NavigationEvent(NavigationEvent::Type::SelectItem));
                    // }
                    return;
                    break;
                case 45: // - (minus)
                    if (isShiftPressed)
                    {
                        m_eventBus.publish(NavigationEvent(NavigationEvent::Type::IncreaseAuxValue));
                        m_eventBus.publish(SystemEvent(SystemEvent::KeyDown));
                    }
                    // else
                    // {
                    //     m_eventBus.publish(NavigationEvent(NavigationEvent::Type::HierarchyDown));
                    //     m_eventBus.publish(NavigationEvent(NavigationEvent::Type::SelectItem));
                    // }
                    return;
                    break;
                default:
                    break;
            }
            
            for(int j = 0; j < m_editKeys.size(); ++j) 
            {
                if (currentChar == m_editKeys[j]) {
                    if(isShiftPressed){
                        m_eventBus.publish(EditModeEvent(j + 8));
                    }
                    else {
                        m_eventBus.publish(EditModeEvent(j));
                    }
                    m_eventBus.publish(SystemEvent(SystemEvent::KeyDown));
                    return;
                }
            }
            
    
            for(int j = 0; j < m_mediaKeys.size(); ++j) 
            {
                int mediaSlotId = (m_registry.inputMappings().bank * MEDIA_BUTTON_COUNT) + j;
                if (currentChar == m_mediaKeys[j]) {
                    if(isShiftPressed){
                        m_eventBus.publish(MediaSlotEvent(mediaSlotId, false)); // do not trigger playback
                    }
                    else {
                        m_eventBus.publish(MediaSlotEvent(mediaSlotId));
                    }
                    m_eventBus.publish(SystemEvent(SystemEvent::KeyDown));
                    return;
                }
            }
        }
        */

        // quick solution to get analogInput[0]
        float a0 = float(deviceBuffer.analogInput[0]) / 1024.0;
        if (a0 < 0.01) a0 = 0.0;
        else if (a0 > 0.99) a0 = 1.0;
        m_registry.settings().analog0 = a0;
    }
}
