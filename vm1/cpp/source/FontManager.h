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