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
            MT_MediaFiles,
            MT_LiveInputs,
            MT_Shaders,
        MT_PlaybackSelection,
        MT_Effects,
            MT_CustomFx,
            MT_ChromaKey,
            MT_ColorCorrection,
            MT_BlendMode,
        MT_Outputs,
            MT_PlaneSettings,
                MT_HdmiSelection,
                MT_Mask,
                MT_Mapping,
        MT_DeviceSettings,
        MT_NetworkInfo,
        MT_SettingsSelection,
        MT_ButtonMatrix
    };

public:
    MenuSystem() = delete;
    explicit MenuSystem(UI&  ui, Registry& registry, EventBus& eventBus);
    void reset();
    void render();

private:
    void subscribeToEvents();
    void createMenus();
    void setMenu(MenuType menuType);
    void showPopupMessage(const std::string& message);
    void handlePopupMessage();
    void handleMediaAndEditButtons();
    void handleUpAndDownKeys();
    void handleBankSwitching();
    void handleMenuHierachyNavigation(const MenuItem *menuItem);

private:
    void StartupScreen(int id, int* selectedIdx);
    void InfoScreen(int id, int* selectedIdx);

    void InputSelection(int id, int* selectedIdx);
    void FileSelection(int id, int* selectedIdx);
    void LiveInputSelection(int id, int* selectedIdx);
    void ShaderSelection(int id, int* selectedIdx);

    void PlaybackSettings(int id, int* selectedIdx);
    
    void Effects(int id, int* selectedIdx);
    void CustomFx(int id, int* selectedIdx);
    void ChromaKey(int id, int* selectedIdx);
    void ColorCorrection(int id, int* selectedIdx);
    void BlendMode(int id, int* selectedIdx);

    void OutputPlanes(int id, int* selectedIdx);
    void PlaneSettings(int id, int* selectedIdx);
    void HdmiSelection(int id, int* selectedIdx);
    void Mask(int id, int* selectedIdx);
    void Mapping(int id, int* selectedIdx);
    
    void NetworkInfo(int id, int* selectedIdx);
    void GlobalSettings(int id, int* selectedIdx);
    void DeviceSettings(int id, int* focusedIdx);
    void ButtonMatrix(int id, int* focusedIdx);

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

    std::vector<std::pair<char, Color>> m_buttonTexts = {{'Q', Color()}, {'W', Color()}, {'E', Color()}, {'R', Color()}, {'T', Color()}, {'Y', Color()}, {'U', Color()}, {'I',Color()},
                                                         {'A', Color()}, {'S', Color()}, {'D', Color()}, {'F', Color()}, {'G', Color()}, {'H', Color()}, {'J', Color()}, {'K',Color()}};
};
