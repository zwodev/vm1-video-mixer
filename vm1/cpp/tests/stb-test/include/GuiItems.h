#pragma once

#include <string>
#include <span>

class GuiRenderer;

/**
 * @class GuiItem
 * @brief Abstract base class for all GUI items.
 *
 * Provides a common interface for GUI elements that can be rendered.
 */
struct GuiItem
{
    virtual ~GuiItem() = default;
    virtual void render(GuiRenderer &renderer) const = 0;
};

struct GuiTitle : public GuiItem
{
    std::string text;
    GuiTitle(const std::string &t);
    void render(GuiRenderer &renderer) const override;
};

struct GuiSlider : public GuiItem
{
    std::string text;
    float value = 0.0f;
    GuiSlider(const std::string &t, float v);
    void render(GuiRenderer &renderer) const override;
};

struct GuiMenu : public GuiItem
{
private:
    // std::span<const char *const> menuItems;
    std::vector<std::string> menuItems;

    int focused;
    int active;
    bool selectionEnabled;

public:
    GuiMenu(std::vector<std::string> menuItems, int focused, int active = -1, int enableSelection = false);
    void render(GuiRenderer &renderer) const override;
};

struct GuiPng : public GuiItem
{
private:
    std::string fileName;

public:
    GuiPng(std::string fileName);
    void render(GuiRenderer &renderer) const override;
};