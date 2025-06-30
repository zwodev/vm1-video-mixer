#include <iostream>
#include <string.h>
#include <lgpio.h>
#include <chrono>

int main()
{
    std::cout << "I2C Test" << std::endl;
    int h = lgGpiochipOpen(0); // Usually GPIO chip 0 is correct
    if (h < 0)
    {
        std::cerr << "Failed to open gpiochip\n";
        return 1;
    }

    int i2c_handle = lgI2cOpen(1, 0x08, 0); // bus 1, address 0x08
    if (i2c_handle < 0)
    {
        std::cerr << "Failed to open I2C device\n";
        return 1;
    }

    const auto frame_duration = std::chrono::milliseconds(2000);
    auto elapsed_time_millis = std::chrono::steady_clock::now();

    while (true)
    {
        auto current_time = std::chrono::steady_clock::now();
        if (current_time - frame_duration >= elapsed_time_millis)
        {
            elapsed_time_millis = current_time;
            std::cout << "requesting i2c..." << std::endl;
            char buf[32] = {0};
            int count = lgI2cReadDevice(i2c_handle, buf, 32);
            if (count < 0)
            {
                std::cerr << "Failed to read from I2C device\n";
            }
            else
            {
                std::cout << "Received " << count << " bytes: ";
                for (int i = 0; i < count; ++i)
                    std::cout << buf[i];
                std::cout << std::endl;
            }
        }
    }

    lgI2cClose(i2c_handle);
    lgGpiochipClose(h);
    return 0;
}
