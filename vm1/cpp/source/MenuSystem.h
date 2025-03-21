#pragma once

#include <imgui.h>
#include <vector>
#include <string>
#include "Registry.h"
#include "MenuEntry.h"

class MenuSystem {
private:
    Registry& m_registry;
    int currentSelection;
    std::unique_ptr<SubmenuEntry> rootMenu;
    MenuEntry* currentMenu;
    MenuEntry* previousMenu;
    void ColoredText(const std::string& label, ImVec4 textColor, ImVec4 bgColor);

public:
    MenuSystem(Registry& registry);
    void render();
};
