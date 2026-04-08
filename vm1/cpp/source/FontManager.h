/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include "imgui.h"

struct FontManager
{
    ImFont *font_std = nullptr;
    ImFont *font_big = nullptr;

    static FontManager &GetInstance()
    {
        static FontManager instance;
        return instance;
    }

    // Delete copy constructor and assignment operator to enforce singleton pattern
    FontManager(const FontManager &) = delete;
    FontManager &operator=(const FontManager &) = delete;

private:
    FontManager() = default; // Private constructor
};