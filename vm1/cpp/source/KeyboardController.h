
#pragma once

#include <msgpack.hpp>
#include <functional>

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
struct ControllerState
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

class KeyboardController
{

public:
    KeyboardController() = default;
    ~KeyboardController() = default;

    bool connect(const std::string &port);
    void disconnect();
    void send(const ControllerState &state);

private:
    int m_fd = -1;
    size_t m_lastHash = 0;
};