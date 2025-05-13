#include <iostream>
#include <msgpack.hpp>
#include <vector>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>

enum button_state
{
    NONE,
    EMPTY,
    FILE_ASSET,
    LIVECAM,
    SHADER,
    FILE_ASSET_ACTIVE,
    LIVECAM_ACTIVE,
    SHADER_ACTIVE,
};

// Helper: Set up serial port
int open_serial(const char *port)
{
    int fd = open(port, O_RDWR);
    if (fd < 0)
        return -1;

    termios tty;
    memset(&tty, 0, sizeof tty);
    tcgetattr(fd, &tty);

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

    tcsetattr(fd, TCSANOW, &tty);
    return fd;
}

int main()
{
    const char *port = "/dev/ttyACM0";
    int fd = open_serial(port);
    if (fd < 0)
    {
        std::cerr << "Failed to open serial port\n";
        return 1;
    }

    // Set Button States
    std::vector<button_state> button_states = {
        button_state::NONE,
        button_state::NONE,

        button_state::NONE,
        button_state::NONE,
        button_state::NONE,
        button_state::NONE,
        button_state::NONE,
        button_state::NONE,
        button_state::NONE,
        button_state::NONE,

        button_state::EMPTY,
        button_state::EMPTY,
        button_state::EMPTY,
        button_state::FILE_ASSET_ACTIVE,

        button_state::FILE_ASSET,
        button_state::FILE_ASSET,
        button_state::FILE_ASSET,
        button_state::LIVECAM,

        button_state::NONE,

        button_state::LIVECAM,
        button_state::FILE_ASSET,
        button_state::FILE_ASSET_ACTIVE,
        button_state::FILE_ASSET,

        button_state::EMPTY,
        button_state::EMPTY,
        button_state::EMPTY,
        button_state::EMPTY,

    };

    // Convert button_states to integers for MessagePack
    std::vector<int> button_states_as_int;
    for (const auto &state : button_states)
        button_states_as_int.push_back(static_cast<int>(state));

    // Pack data with MessagePack
    msgpack::sbuffer buffer;
    // msgpack::pack(buffer, leds);
    msgpack::pack(buffer, button_states_as_int);

    // Send to Pico
    write(fd, buffer.data(), buffer.size());
    close(fd);

    std::cout << "Sent MessagePack.\n";

    return 0;
}
