/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

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