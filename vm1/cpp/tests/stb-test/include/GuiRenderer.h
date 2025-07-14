#pragma once
#include <string>
#include <span>
#include "StbRenderer.h"

class GuiRenderer
{
private:
    StbRenderer stbRenderer;
    int width;
    int height;

public:
    static const int fontSizeTitle = 32;
    static const int fontSizeText = 16;

    GuiRenderer(int width, int height);
    StbRenderer *getStbRenderer();

    void clear();
    void drawTitle(const std::string &text);
    void drawSlider(const std::string &text, float value);
    void drawMenu(const std::span<const std::string> menuItems, int focused, int active = -1, bool enableSelection = false);
    void drawPng(std::string filename);
};