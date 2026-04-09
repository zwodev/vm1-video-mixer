/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
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
    MenuState(int fIdx, std::function<void()> func, std::string name = "") {
        this->fIdx = fIdx;
        this->func = func; 
        this->name = name;
    }

    int fIdx = 0;
    std::function<void()> func;
    std::string name;
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

    // TODO: Why do we have this? It is never used at the moment.
    enum SubMenuType
    {
        SMT_None,
        SMT_Mapping,
        SMT_Mask
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
    std::string currentDirectoryPath();


    bool SubMenu(const std::string& label, std::function<void()> func, SubMenuType subMenuType = SMT_None);
    bool SubDir(const std::string& label, std::function<void()> func, SubMenuType subMenuType = SMT_None);
    void MediaPreview(const std::string& filename);
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
    void CustomEffectShaderSelection();
    void EffectSelection();
    void EffectControl();
  

    // Output
    void OutputMenu();
    void Mask();
    void Mapping();
    // void HdmiSelection();

    // Network
    void NetworkMenu();

    // Global Settings
    void GlobalSettingsMenu();

    // Device Settings
    void DeviceSettingsMenu();

    // Button Matrix
    void ButtonMatrixMenu();



private:
    UI &m_ui;
    Registry &m_registry;
    EventBus &m_eventBus;
    
    int m_id = 0;
    int m_outputPlaneId = 0;
    int m_selectedVertexId = 0;
    std::string m_currentMediaSlotId = "";
    int m_currentMediaSlotPlaneId = 0;

    std::string m_effectName;
    int m_focusedIdx = 0;

    // TODO: Save all this in a MenuState instance and only use m_currentMenuPath
    MenuType m_currentMenuType = MT_StartupScreen;
    SubMenuType m_currentSubMenuType = SMT_None;
    std::string m_currentMenuName;
    std::function<void()> m_currentMenuFunc;
    std::vector<MenuState> m_currentMenuPath;

    // TODO: Put all this into a media preview struct.
    // Media Preview
    ImageBuffer m_mediaPreviewImageBuffer;
    std::string m_previewMediaFileNameOld;
    int m_mediaPreviewFrameIndex = 0;
    
    bool m_launchPopup = false;
    std::string m_lastPopupMessage;

    bool m_showStringInputDialog = false;
    int m_stringInputDialogCursorIdx = 0;
    std::string m_stringInputDialogString = "";

    std::vector<std::pair<char, Color>> m_buttonTexts = {{'Q', Color()}, {'W', Color()}, {'E', Color()}, {'R', Color()}, {'T', Color()}, {'Y', Color()}, {'U', Color()}, {'I',Color()},
                                                         {'A', Color()}, {'S', Color()}, {'D', Color()}, {'F', Color()}, {'G', Color()}, {'H', Color()}, {'J', Color()}, {'K',Color()}};
};
