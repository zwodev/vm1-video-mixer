#pragma once

#include <imgui.h>
#include <vector>
#include <string>

class MenuSystem {
private:
    std::vector<std::vector<std::string>> menus;
    int currentMenu;
    int currentSelection;

public:
    MenuSystem();
    void render();
};
