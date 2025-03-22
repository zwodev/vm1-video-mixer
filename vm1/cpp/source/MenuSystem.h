#pragma once

#include <imgui.h>
#include <vector>
#include <string>
#include "Registry.h"
#include "MenuEntry.h"

class MenuSystem
{
public:
    enum MenuType {
        MT_Logo,
        MT_InputSelection
    };

public:
    MenuSystem(Registry &registry);
    void render();
    

private:
    void buildMenuStructure();
    void buildInputMenuStructure(std::unique_ptr<SubmenuEntry>& rootEntry);
    void setMenu(MenuType menuType);
    void handleKeyboardShortcuts();

private:
    Registry &m_registry;
    std::map<MenuType, std::unique_ptr<MenuEntry>> m_menus;
    int m_id = -1;
    int m_bank = 0;
    int m_currentSelection = 0;
    MenuEntry* m_rootMenu = nullptr;
    MenuEntry* m_currentMenu = nullptr;
    MenuEntry* m_previousMenu = nullptr;

    // Keyboard Shortcuts
    std::vector<ImGuiKey> m_keyboardShortcuts = {   ImGuiKey_Q, ImGuiKey_W, ImGuiKey_E, ImGuiKey_R, 
                                                    ImGuiKey_T, ImGuiKey_Z, ImGuiKey_U, ImGuiKey_I, 
                                                    ImGuiKey_A, ImGuiKey_S, ImGuiKey_D, ImGuiKey_F, 
                                                    ImGuiKey_G, ImGuiKey_H, ImGuiKey_J, ImGuiKey_K  
                                                };
};
