#include <string>
#include "GuiItems.h"
#include "GuiRenderer.h"

// ==================================================================
// Gui Label

GuiTitle::GuiTitle(const std::string &t) : text(t) {}

void GuiTitle::render(GuiRenderer &renderer) const
{
    renderer.drawTitle(text);
}

// ==================================================================
// Gui Slider

GuiSlider::GuiSlider(const std::string &t, float v) : text(t), value(v)
{
}
void GuiSlider::render(GuiRenderer &renderer) const
{
    renderer.drawSlider(text, value);
}

// ==================================================================
// Gui Menu

GuiMenu::GuiMenu(std::vector<std::string> menuItems, int focused, int active, int enableSelection) : menuItems(menuItems),
                                                                                                     focused(focused),
                                                                                                     active(active),
                                                                                                     selectionEnabled(enableSelection)
{
}

void GuiMenu::render(GuiRenderer &renderer) const
{
    renderer.drawMenu(menuItems, focused, active, selectionEnabled);
}

// ==================================================================
// Gui Png (assuming Fullscreen png at the moment)

GuiPng::GuiPng(std::string filename) : fileName(filename)
{
}

void GuiPng::render(GuiRenderer &renderer) const
{
    renderer.drawPng(fileName);
}
