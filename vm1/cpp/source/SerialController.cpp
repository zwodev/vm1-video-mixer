#include "SerialController.h"

#include <string>
#include <iostream>
// #include <vector>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>

// Helper: Set up serial port
bool SerialController::connect(const std::string &port)
{
    m_fd = open(port.c_str(), O_RDWR);
    if (m_fd < 0)
        return false;

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

void SerialController::disconnect()
{
    close(m_fd);
    m_fd = -1;
}

void SerialController::send(const VM1DeviceState &vm1DeviceState)
{
    if (m_fd < 0)
    {
        return;
    }

    // Do not send state when nothing has changed (aka hash is the same)
    size_t hash = vm1DeviceState.hash();
    if (hash == m_lastHash)
        return;

    m_lastHash = hash;

    printf("bank: %d\n", vm1DeviceState.bank);
    // printf("backward button: %d\n", controllerState.backward);
    // printf("forward button: %d\n", controllerState.forward);
    // printf("fn button: %d\n", controllerState.fn);
    // printf("edit buttons:\n");
    // for (uint8_t i = 0; i < 8; i++)
    // {
    //     printf("%d : %d\r\n", i, controllerState.edit[i]);
    // }
    // printf("media buttons:\n");
    // for (uint8_t i = 0; i < 16; i++)
    // {
    //     printf("%d : %d\r\n", i, controllerState.media[i]);
    // }

    // Send to Pico
    write(m_fd, (char *)&vm1DeviceState, sizeof(VM1DeviceState));
}