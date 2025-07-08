#include "GuiRenderer.h"

GuiRenderer::GuiRenderer(int width, int height) : width(width), height(height), stbRenderer(width, height)
{
}

StbRenderer *GuiRenderer::getStbRenderer()
{
    return &stbRenderer;
}

void GuiRenderer::clear()
{
    stbRenderer.clear();
}

void GuiRenderer::drawTitle(const std::string &text)
{
    stbRenderer.drawText(text, 0, 0, fontSizeTitle, COLOR::FOREGROUND);
}

void GuiRenderer::drawSlider(const std::string &text, float value)
{
    stbRenderer.drawRect(20, 20, 20 + value * 10, 30);
}

void GuiRenderer::drawMenu(const std::span<const std::string> menuItems, int focused, int active, bool enableSelection)
{
    int x = 0;  // temp
    int y = 20; // temp

    int lineHeightPx = stbRenderer.getFontLineHeight(fontSizeText);
    int menuHeightPx = height - y;
    int maxVisibleLines = menuHeightPx / lineHeightPx;
    int visibleLines = std::clamp(maxVisibleLines, 0, int(menuItems.size()));

    int firstLine = std::clamp(focused - visibleLines / 2,
                               0,
                               std::max(0, int(menuItems.size()) - visibleLines));

    std::cout << "FirstLine: " << firstLine
              << " focused: " << focused
              << " visible lines: " << visibleLines << "/" << maxVisibleLines
              << " no of entries: " << int(menuItems.size())
              << std::endl;

    for (int i = firstLine; i < (visibleLines + firstLine) && i < menuItems.size(); ++i)
    {
        std::string item = menuItems[i];

        c textColor = COLOR::FOREGROUND;
        if (i == focused)
        {
            stbRenderer.drawRect(x, y + lineHeightPx * i - (firstLine * lineHeightPx) - 2, width, lineHeightPx + 1, COLOR::FOREGROUND);
            textColor = COLOR::BLACK;
        }

        if (enableSelection)
        {
            item = i == active ? "[x]" + item : "[ ]" + item;
        }
        std::cout << "[" << i << "] " << item << std::endl;
        stbRenderer.drawText(item, x, y + lineHeightPx * i - (firstLine * lineHeightPx), fontSizeText, textColor);
    }
}

void GuiRenderer::drawPng(std::string fileName)
{
    stbRenderer.drawPng(fileName);
}