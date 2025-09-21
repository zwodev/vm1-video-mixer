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
    if(connectSerial(port)) 
    {
        std::cout << "Connected to VM1-Device via Serial Connection." << std::endl;
        isProVersion = true;
    }
    else 
    {
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
    }

    m_registry.settings().isProVersion = isProVersion;
    return true;
}

bool DeviceController::connectSerial(const std::string& port)
{
    m_fd = open(port.c_str(), O_RDWR);
    if (m_fd < 0) {
        return false;
    }
    termios tty;
    memset(&tty, 0, sizeof tty);
    tcgetattr(m_fd, &tty);

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_lflag = 0;
    tty.c_iflag = 0;
    tty.c_oflag = 0;

    tcsetattr(m_fd, TCSANOW, &tty);
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
    if (m_fd >= 0)
    {
        close(m_fd);
        m_fd = -1;
    }

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

    if (m_fd >= 0)
    {
        write(m_fd, (char *)&vm1DeviceState, sizeof(VM1DeviceState));
    } 
    else if (m_i2c_handle >= 0) 
    {
        lgI2cWriteDevice(m_i2c_handle, reinterpret_cast<const char*>(&vm1DeviceState), sizeof(VM1DeviceState));
    }

}

void DeviceController::requestVM1DeviceBuffer() 
{
    if(m_i2c_handle < 0) return;
    
    DeviceBuffer deviceBuffer;
    int count = lgI2cReadDevice(m_i2c_handle, reinterpret_cast<char*>(&deviceBuffer), sizeof(DeviceBuffer));
    if (count < 0)
    {
        std::cerr << "Failed to read from I2C device\n";
    }
    else
    {
        // std::cout << "Received " << count << " bytes: " << std::endl;
        // std::cout << "Rotary 0: " <<  deviceBuffer.rotary_0 << std::endl;
        // std::cout << "Rotary 1: " <<  deviceBuffer.rotary_1 << std::endl;
        // std::cout << "Shift pressed: " <<  deviceBuffer.shiftPressed << std::endl;
        // for(uint8_t i = 0; i < sizeof(deviceBuffer.buttons); ++i) {
        //     std::cout << deviceBuffer.buttons[i] << "(" << static_cast<int>(deviceBuffer.buttons[i]) << ")\t";
        // }
        // std::cout << std::endl;
        
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
                default:
                    break;
            }
            
            for(int j = 0; j < m_editKeys.size(); ++j) 
            {
                if (currentChar == m_editKeys[j]) {
                    m_eventBus.publish(EditModeEvent(j));
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

        
        
    }
}
