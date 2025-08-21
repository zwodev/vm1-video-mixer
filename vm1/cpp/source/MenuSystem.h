/*
 * Copyright (c) 2025 Julian Jungel & Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include <vector>
#include <string>
#include "Registry.h"
#include "EventBus.h"
#include "UI.h"

struct MenuItem {
    std::string label;
    std::vector<MenuItem> children;
    std::function<void(int, int*)> func;
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
        MT_DeviceSettings,
        MT_NetworkInfo,
        MT_SettingsSelection
    };

public:
    MenuSystem() = delete;
    explicit MenuSystem(UI&  ui, Registry& registry, EventBus& eventBus);
    void render();

private:
    void subscribeToEvents();
    void createMenus();
    void setMenu(MenuType menuType);
    void launchPopup(const std::string& message);
    void handlePopupMessage();
    void handleMediaAndEditButtons();
    void handleUpAndDownKeys();
    void handleBankSwitching();
    void handleMenuHierachyNavigation(const MenuItem *menuItem);

private:
    void StartupScreen(int id, int* selectedIdx);
    void FileSelection(int id, int* selectedIdx);
    void LiveInputSelection(int id, int* selectedIdx);
    void PlaybackSettings(int id, int* selectedIdx);
    void NetworkInfo(int id, int* selectedIdx);
    void GlobalSettings(int id, int* selectedIdx);
    void DeviceSettings(int id, int* focusedIdx);

private:
    Registry &m_registry;
    EventBus &m_eventBus;
    UI &m_ui;
    int m_id = 0;
    int m_focusedIdx = 0;
    std::vector<int> m_currentMenuPath;
    std::map<MenuType, MenuItem> m_menus;
    MenuType m_currentMenuType = MT_InputSelection;
    bool m_launchPopup = false;
    std::string m_lastPopupMessage;
};
