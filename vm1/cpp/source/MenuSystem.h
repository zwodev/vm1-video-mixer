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

struct MenuState {
    MenuState(int fIdx, std::function<void()> func) {
        this->fIdx = fIdx;
        this->func = func; 
    }

    int fIdx = 0;
    std::function<void()> func;
};

class MenuSystem
{
public:
    enum MenuType
    {
        MT_StartupScreen,
        MT_InfoMenu,
        MT_SourceMenu,
        MT_ControlMenu,
        MT_FxMenu,
        MT_OutputMenu,
        MT_DeviceSettingsMenu,
        MT_NetworkMenu,
        MT_GlobalSettingsMenu,
        MT_ButtonMatrixMenu
    };

public:
    MenuSystem() = delete;
    explicit MenuSystem(UI&  ui, Registry& registry, EventBus& eventBus);
    void reset();
    void render();

private:
    void subscribeToEvents();
    void setMenu(MenuType menuType);
    void showPopupMessage(const std::string& message);
    void handlePopupMessage();
    void handleStringInputDialog();
    void handleMediaAndEditButtons();
    void handleUpAndDownKeys();
    void handleBankSwitching();
    void handlePlaneSwitching();
    void goUpHierachy();
    void handleMenuHierachyNavigation();

private:
    bool SubMenu(const std::string& label, std::function<void()> func);
    void ClearSlot();

    void StartupScreen();

    // Info
    void InfoMenu();
    
    // Source
    void SourceMenu();
    void FileSelection();
    void LiveInputSelection();
    void ShaderSelection();
    
    //.Control
    void ControlMenu();

    // FX
    void FxMenu();
    void EffectSelection();
    void EffectControl();

    // Output
    void OutputMenu();
    void Mask();
    void Mapping();
    void HdmiSelection();

    // Network
    void NetworkMenu();

    // Global Settings
    void GlobalSettingsMenu();

    // Device Settings
    void DeviceSettingsMenu();

    // Button Matrix
    void ButtonMatrixMenu();



private:
    Registry &m_registry;
    EventBus &m_eventBus;
    UI &m_ui;
    
    int m_id = 0;
    int m_planeIdx = 0;
    int m_effectIdx = 0;
    int m_focusedIdx = 0;

    MenuType m_currentMenuType = MT_StartupScreen;
    std::function<void()> m_currentMenuFunc;
    std::vector<MenuState> m_currentMenuPath;

    bool m_launchPopup = false;
    std::string m_lastPopupMessage;

    bool m_showStringInputDialog = false;
    int m_stringInputDialogCursorIdx = 0;
    std::string m_stringInputDialogString = "";

    std::vector<std::pair<char, Color>> m_buttonTexts = {{'Q', Color()}, {'W', Color()}, {'E', Color()}, {'R', Color()}, {'T', Color()}, {'Y', Color()}, {'U', Color()}, {'I',Color()},
                                                         {'A', Color()}, {'S', Color()}, {'D', Color()}, {'F', Color()}, {'G', Color()}, {'H', Color()}, {'J', Color()}, {'K',Color()}};
};
