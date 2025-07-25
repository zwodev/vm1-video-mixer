#include <iostream>
#include <string.h>
#include <lgpio.h>
#include <chrono>

enum ButtonState : uint8_t
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

#pragma pack(1)
struct VM1DeviceState
{
    uint8_t bank;
    ButtonState forward = ButtonState::NONE;
    ButtonState backward = ButtonState::NONE;
    ButtonState fn = ButtonState::FILE_ASSET;
    ButtonState edit[8] = {ButtonState::NONE};
    ButtonState media[16] = {ButtonState::NONE};

    template <typename T>
    inline void hashCombine(std::size_t &seed, const T &v) const
    {
        seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    size_t hash() const
    {
        size_t seed = 0;
        hashCombine(seed, bank);
        hashCombine(seed, forward);
        hashCombine(seed, backward);
        hashCombine(seed, fn);

        for (auto state : edit)
        {
            hashCombine(seed, state);
        }

        for (auto state : media)
        {
            hashCombine(seed, state);
        }

        return seed;
    }
};
#pragma pack()

#pragma pack(1)
struct DeviceBuffer
{
  char buttons[8];
  int32_t rotary_0;
  int32_t rotary_1;
};
#pragma pack()


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
    
    const auto frame_duration = std::chrono::milliseconds(1000/60);
    auto elapsed_time_millis = std::chrono::steady_clock::now();
    
    DeviceBuffer deviceBuffer;
    while (true)
    {
        auto current_time = std::chrono::steady_clock::now();
        if (current_time - frame_duration >= elapsed_time_millis)
        {
            // send a command:
            // ===============
            VM1DeviceState deviceState;
            deviceState.backward = ButtonState::FILE_ASSET;
            deviceState.forward = ButtonState::FILE_ASSET_ACTIVE;
            deviceState.fn = ButtonState::SHADER;
            deviceState.bank = (deviceState.bank + 1) % 6;
            lgI2cWriteDevice(i2c_handle, reinterpret_cast<const char*>(&deviceState), sizeof(deviceState));

            // send a request:
            // ===============
            elapsed_time_millis = current_time;
            std::cout << "requesting i2c..." << std::endl;
            int count = lgI2cReadDevice(i2c_handle, reinterpret_cast<char*>(&deviceBuffer), sizeof(deviceBuffer));
            if (count < 0)
            {
                std::cerr << "Failed to read from I2C device\n";
            }
            else
            {
                std::cout << "Received " << count << " bytes: " << std::endl;
                std::cout << "Rotary 0: " <<  deviceBuffer.rotary_0 << std::endl;
                std::cout << "Rotary 1: " <<  deviceBuffer.rotary_1 << std::endl;
                for(uint8_t i = 0; i < sizeof(deviceBuffer.buttons); ++i) {
                    std::cout << deviceBuffer.buttons[i] << "(" << static_cast<int>(deviceBuffer.buttons[i]) << ")\t";
                }
                std::cout << std::endl;
            }
        }
    }

    lgI2cClose(i2c_handle);
    lgGpiochipClose(h);
    return 0;
}
