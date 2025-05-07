#include <iostream>
#include <msgpack.hpp>
#include <vector>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>

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

    // Create RGB values for 15 LEDs
    std::vector<std::vector<int>> leds(15, std::vector<int>(3));
    for (int i = 0; i < 15; ++i)
    {
        leds[i][0] = 255; // Red
        leds[i][1] = 0;   // Green
        leds[i][2] = 0;   // Blue
        // leds[i][0] = (i * 17) % 256;       // Red
        // leds[i][1] = (255 - i * 10) % 256; // Green
        // leds[i][2] = (i * 5) % 256;        // Blue
    }

    // Pack data with MessagePack
    msgpack::sbuffer buffer;
    msgpack::pack(buffer, leds);

    // Send to Pico
    write(fd, buffer.data(), buffer.size());
    close(fd);

    std::cout << "Sent MessagePack with 15 LED colors.\n";

    return 0;
}
