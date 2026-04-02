#pragma once

#include <vector>
#include <SDL3/SDL.h>
#include "EventBus.h"
#include "Registry.h"

class KeyboardController
{
public:
    KeyboardController() = delete;
    explicit KeyboardController(Registry& registry, EventBus& eventBus);
    virtual ~KeyboardController() = 0;

protected:
    Registry& m_registry;
    EventBus& m_eventBus;
};