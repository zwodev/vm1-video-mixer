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
    SubmenuEntry* rootMenu;
    MenuEntry* currentMenu = nullptr;
    MenuEntry* previousMenu = nullptr;
    void ColoredText(const std::string& label, ImVec4 textColor, ImVec4 bgColor);

public:
    MenuSystem(Registry& registry);
    void render();
};
