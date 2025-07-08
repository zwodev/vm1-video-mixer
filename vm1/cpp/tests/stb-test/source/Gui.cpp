#include "Gui.h"
#include "GuiStates.h"
#include "Settings.h"
#include "StbRenderer.h"

Gui::Gui() : redrawNeeded(true)
{
}

void Gui::render(GuiState &activeState, Settings &settings)
{
    activeState.describeUI();
}

void Gui::requestRedraw()
{
    redrawNeeded = true;
}

bool Gui::shouldRedraw()
{
    bool tmp = redrawNeeded;
    redrawNeeded = false;
    return tmp;
}

void Gui::addLabel(const std::string &text)
{
    sceneGraph.push_back(std::make_unique<GuiTitle>(text));
}

void Gui::addSlider(const std::string &text, float value)
{
    sceneGraph.push_back(std::make_unique<GuiSlider>(text, value));
}

void Gui::addMenu(std::span<const char *const> menuItems_, int focused, int active, int enableSelection)
{
    std::vector<std::string> menuItems;
    for (const char *item : menuItems_)
        menuItems.push_back(std::string(item));

    sceneGraph.push_back(std::make_unique<GuiMenu>(menuItems, focused, active, enableSelection));
}

void Gui::addMenu(std::vector<std::string> menuItems, int focused, int active, int enableSelection)
{
    sceneGraph.push_back(std::make_unique<GuiMenu>(menuItems, focused, active, enableSelection));
}

void Gui::addPng(std::string fileName)
{
    sceneGraph.push_back(std::make_unique<GuiPng>(fileName));
}

void Gui::clearSceneGraph()
{
    sceneGraph.clear();
}

// const std::vector<GuiItem> &Gui::getSceneGraph() const
// {
// }