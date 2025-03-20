#pragma once

#include <imgui.h>
#include <vector>
#include <string>
#include "MenuEntry.h"

class MenuSystem {
private:
    int currentSelection;

    SubmenuEntry* rootMenu;
    MenuEntry* currentMenu = nullptr;
    MenuEntry* previousMenu = nullptr;
    void ColoredText(const std::string& label, ImVec4 textColor, ImVec4 bgColor);

public:
    MenuSystem();
    void render();
};
