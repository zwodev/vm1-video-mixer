#pragma once

#include <imgui.h>
#include <vector>
#include <string>
#include "Registry.h"
#include "MenuEntry.h"

class MenuSystem
{
public:
    MenuSystem(Registry &registry);
    void render();

private:
    void buildMenuStructure();
    void buildInputMenuStructure(std::unique_ptr<SubmenuEntry>& rootEntry);

private:
    Registry &m_registry;
    int currentSelection;
    std::unique_ptr<SubmenuEntry> rootMenu;
    MenuEntry *currentMenu;
    MenuEntry *previousMenu;
};
