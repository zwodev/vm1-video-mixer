#include "KeyboardController.h"

#include <string>
#include <iostream>
//#include <vector>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>

// Helper: Set up serial port
bool KeyboardController::connect(const std::string& port)
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

void KeyboardController::disconnect()
{
    close(m_fd);
    m_fd = -1;
}

void KeyboardController::send(const ControllerState& controllerState)
{
    if (m_fd < 0) { return; }

    // Do not send state when nothing has changed (aka hash is the same)
    size_t hash = controllerState.hash();
    if (hash == m_lastHash)
        return;

    m_lastHash = hash;

    // Pack data with MessagePack
    msgpack::sbuffer buffer;
    // msgpack::pack(buffer, leds);
    msgpack::pack(buffer, controllerState);

    // Send to Pico
    write(m_fd, buffer.data(), buffer.size());
}