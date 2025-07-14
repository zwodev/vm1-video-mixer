#pragma once

#include <vector>
#include <string>
#include <span>
#include <memory>
#include "GuiItems.h"

class GuiState;
struct Settings;

/**
 * @class Gui
 * @brief Manages the graphical user interface (GUI) elements and rendering.
 *
 * The Gui class is responsible for maintaining a scene graph of GUI items,
 * handling redraw requests, and providing methods to add and manage GUI components
 * such as labels and sliders. It also provides rendering functionality and
 * manages the state of the GUI.
 */
class Gui
{
private:
    bool redrawNeeded;

public:
    std::vector<std::unique_ptr<GuiItem>> sceneGraph;

    Gui();
    void render(GuiState &activeState, Settings &settings);
    void requestRedraw();
    bool shouldRedraw();

    void addLabel(const std::string &text);
    void addSlider(const std::string &text, float value);
    void addMenu(std::span<const char *const> menuItems, int focused, int active = -1, int enableSelection = false);
    void addMenu(std::vector<std::string> menuItems, int focused, int active = -1, int enableSelection = false);
    void addPng(std::string fileName);

    void clearSceneGraph();
    // const std::vector<GuiItem> &getSceneGraph() const;
};
