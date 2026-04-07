/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include <string>
#include <functional>
#include <stdint.h>
#include "VM1DeviceDefinitions.h"
#include "EventBus.h"
#include "Registry.h"

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
    MEDIABUTTON_SELECTED,
    YELLOW,
    GREEN,
    BLUE,
    RED
};

enum RotaryButtonIds
{
    PRIMARY_ENCODER_CCW,
    PRIMARY_ENCODER_CW,
    SECONDARY_ENCODER_CCW,
    SECONDARY_ENCODER_CW
};
enum NavigationButtonIds
{
    NAVIGATION_BUTTON_LEFT,
    NAVIGATION_BUTTON_RIGHT
};



#pragma pack(1)
struct VM1DeviceState
{
    uint8_t rotarySensitivity = 5;
    uint8_t bank = 0;
    ButtonState forward = ButtonState::NONE;
    ButtonState backward = ButtonState::NONE;
    ButtonState fn = ButtonState::NONE;
    ButtonState editButtons[EDIT_BUTTON_COUNT] = {ButtonState::NONE};
    ButtonState mediaButtons[MEDIA_BUTTON_COUNT] = {ButtonState::EMPTY};

    template <typename T>
    inline void hashCombine(std::size_t &seed, const T &v) const
    {
        seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    size_t hash() const
    {
        size_t seed = 0;
        hashCombine(seed, rotarySensitivity);
        hashCombine(seed, bank);
        hashCombine(seed, forward);
        hashCombine(seed, backward);
        hashCombine(seed, fn);

        for (auto state : editButtons)
        {
            hashCombine(seed, state);
        }

        for (auto state : mediaButtons)
        {
            hashCombine(seed, state);
        }

        return seed;
    }
};
#pragma pack()

enum EventType : uint8_t {
  EDIT_BUTTON_EVENT,
  MEDIA_BUTTON_EVENT,
  NAVIGATION_BUTTON_EVENT,
  ROTARY_EVENT,
  NO_EVENT
};


#pragma pack(1)
struct ButtonEvent {
  EventType eventType;
  int8_t buttonId;
};
#pragma pack()

#pragma pack(1)
struct DeviceBuffer
{
  ButtonEvent buttonEvents[8];
  bool fnPressed;
  uint16_t analogInput[4];
};
#pragma pack()

class DeviceController
{

public:
    DeviceController(EventBus& eventBus, Registry& registry);
    ~DeviceController();

    bool connect(const std::string& port);
    void disconnect();
    void send(const VM1DeviceState& state);
    void requestVM1DeviceBuffer();

private:
    bool connectI2C();
    int m_i2c_handle = -1;
    int m_gpioHandler = -1;
    size_t m_lastHash = 0;
    EventBus& m_eventBus;
    Registry& m_registry;

    std::vector<char> m_editKeys =  {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i'};
    std::vector<char> m_mediaKeys = {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k',
                                      'z', 'x', 'c', 'v', 'b', 'n', 'm', ','};

};