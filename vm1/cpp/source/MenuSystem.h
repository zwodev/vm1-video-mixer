#pragma once

#include <imgui.h>
#include <vector>
#include <string>
#include "Registry.h"
#include "MenuEntry.h"

class MenuSystem
{
private:
    Registry &m_registry;
    int currentSelection;
    std::unique_ptr<SubmenuEntry> rootMenu;
    MenuEntry *currentMenu;
    MenuEntry *previousMenu;

    enum TextState
    {
        DEFAULT,
        HIGHLIGHT,
        SELECTED,
        ERROR,
        WARNING
    };
    void SetTextSettings(TextState state, ImVec4 &textColor, ImVec4 &bgColor);
    void RenderText(const std::string &label, TextState textState = TextState::DEFAULT);
    void RenderOverlayText(const std::string &text);

public:
    MenuSystem(Registry &registry);
    void render();
};
