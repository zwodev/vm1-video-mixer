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
    std::vector<std::string> splitPath(const std::string& path);
    
    // Popups and Overlays
    void showPopupMessage(const std::string& message);
    void handlePopupMessage();
    void TextInputDialog();
    void FileManagerDialog();
    void FolderSelectionDialog();
    
    // Widgets
    void MediaPreview(const std::string& filename);
    
    // 
    void handleMediaAndEditButtons();
    void handleUpAndDownKeys();
    void handleBankSwitching();
    void handlePlaneSwitching();
    void goUpHierachy();
    void appendStateToMenuPath(MenuState menuState);
    void handleMenuHierachyNavigation();
    std::string currentDirectoryPath();
    
    void setMenu(MenuType menuType);
    bool SubMenu(const std::string& label, std::function<void()> func);
    bool SubDir(const std::string& label, std::function<void()> func);
    void ClearSlot();

    // Start Screen
    void StartupScreen();

    // Info 
    void InfoMenu();
    
    // Source
    void SourceMenu();
    void SelectActiveSourceFolder();
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

    MenuType m_currentMenuType = MT_StartupScreen;
    std::vector<MenuState> m_currentMenuPath;
    std::vector<MenuState> m_nextMenuPath;
    std::string m_effectName;

    bool m_focusActiveSource = false;
    bool m_showActivesSource = false;

    struct MediaSlotData {
        int slotId = 0;
        int planeId = 0;
        std::string slotName;
    };
    MediaSlotData m_activeMediaSlot;

    struct OutputPlaneData {
        int planeId = 0;
        int selectedVertexId = 0;
    };
    OutputPlaneData m_activeOutputPlane;

    struct PreviewData {
        std::string imageFileName;
        int frameIndex = 0;
    };
    PreviewData m_preview;

    struct PopUpData {
        bool show = false;
        std::string message;
    };
    PopUpData m_popUp;

    struct TextInputDialogData {
        int cursorIdx = 0;
        std::string title;
        std::string text;
        std::function<void()> func;
    };
    TextInputDialogData m_textInputDialog;

    struct FileManagerDialogData {
        std::string filename;
    };
    FileManagerDialogData m_fileManagerDialog;

    struct FolderSelectionDialogData {
        std::string title;
        std::string subtitle;
        std::string foldername;
        std::vector<std::filesystem::path> subFolders;
        std::function<void()> func;
    };
    FolderSelectionDialogData m_folderSelectionDialog;

    std::vector<std::pair<char, Color>> m_buttonTexts = {{'Q', Color()}, {'W', Color()}, {'E', Color()}, {'R', Color()}, {'T', Color()}, {'Y', Color()}, {'U', Color()}, {'I',Color()},
                                                         {'A', Color()}, {'S', Color()}, {'D', Color()}, {'F', Color()}, {'G', Color()}, {'H', Color()}, {'J', Color()}, {'K',Color()}};
};
