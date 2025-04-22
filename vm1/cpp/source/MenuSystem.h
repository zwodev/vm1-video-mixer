/*
 * Copyright (c) 2025 Julian Jungel & Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include <imgui.h>
#include <vector>
#include <string>
#include "Registry.h"

struct MenuItem {
    std::string label;
    std::vector<MenuItem> children;
    void (*renderFunc)(Registry*, int, int*) = nullptr;
};

class MenuSystem
{
public:
    enum MenuType
    {
        MT_StartupScreen,
        MT_InfoSelection,
        MT_InputSelection,
        MT_PlaybackSelection,
        MT_SettingsSelection
    };

public:
    MenuSystem(Registry &registry);
    void render();

private:
    void setMenu(MenuType menuType);
    void handleMediaAndEditButtons();
    

private:
    static void HandleUpAndDownKeys(int* selectedIdx, int menuSize);
    static void StartupScreen(Registry* registry, int id, int* selectedIdx);
    static void FileSelection(Registry* registry, int id, int* selectedIdx);
    static void LiveInputSelection(Registry* registry, int id, int* selectedIdx);
    static void PlaybackSettings(Registry* registry, int id, int* selectedIdx);
    static void GlobalSettings(Registry* registry, int id, int* selectedIdx);

private:
    Registry &m_registry;
    int m_id = 0;
    int m_bank = 0;
    int m_selectedIdx = 0;
    std::vector<int> m_currentMenuPath;

    
    std::map<MenuType, MenuItem> m_menus;
    MenuType m_currentMenuType = MT_InputSelection;


    // Keyboard Shortcuts
    std::vector<ImGuiKey> m_keyboardShortcuts = {
        ImGuiKey_A, ImGuiKey_S, ImGuiKey_D, ImGuiKey_F,
        ImGuiKey_G, ImGuiKey_H, ImGuiKey_J, ImGuiKey_K,
        ImGuiKey_Z, ImGuiKey_X, ImGuiKey_C, ImGuiKey_V,
        ImGuiKey_B, ImGuiKey_N, ImGuiKey_M, ImGuiKey_Comma};

    std::vector<ImGuiKey> m_keyboardShortcuts_editButtons = {
        ImGuiKey_Q, ImGuiKey_W, ImGuiKey_E, ImGuiKey_R,
        ImGuiKey_T, ImGuiKey_Y, ImGuiKey_U, ImGuiKey_I};
};
