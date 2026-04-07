/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include "KeyboardController.h"

#include "stdio.h"

KeyboardController::KeyboardController(Registry& registry, EventBus& eventBus) : 
    m_registry(registry), m_eventBus(eventBus)
{
}

KeyboardController::~KeyboardController()
{
}

