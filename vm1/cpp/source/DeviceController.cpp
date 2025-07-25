#include "DeviceController.h"

#include <string>
#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <lgpio.h>

DeviceController::~DeviceController(){
    disconnect();
}

// Helper: Set up serial port
bool DeviceController::connect(const std::string& port)
{
    if(connectSerial(port)) 
    {
        std::cout << "Connected to VM1-Device via Serial Connection." << std::endl;
    }
    else 
    {
        std::cout << "Failed to open Serial Connection, trying I2C..." << std::endl;
        if(connectI2C()) 
        {
            std::cout << "Connected to VM1-Device via I2C." << std::endl;
        } 
        else
        {
            std::cout << "Failed to open I2C, using standard keyboard mode." << std::endl;
            return false;
        }    
    }
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