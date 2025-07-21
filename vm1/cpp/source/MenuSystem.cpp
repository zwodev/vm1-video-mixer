/*
 * Copyright (c) 2025 Julian Jungel & Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include "MenuSystem.h"
#include "UI.h"
#include "NetworkTools.h"

#include <imgui.h>
#include <vector>
#include <string>

const int NUM_BANKS = 4;

MenuSystem::MenuSystem(UI& ui, Registry& registry, EventBus& eventBus) : m_ui(ui), m_registry(registry), m_eventBus(eventBus)
{   
    createMenus();
    subscribeToEvents();
    setMenu(MT_StartupScreen);
}

void MenuSystem::createMenus()
{
    // m_menus[MT_StartupScreen]       =  {"", {}, StartupScreen};
    m_menus[MT_StartupScreen]       =  {"", {}, [=](Registry* r, int id, int* fIdx){StartupScreen(r, id, fIdx);}};
    m_menus[MT_InfoSelection]       =  {"Info", {}};
    m_menus[MT_InputSelection]      =  {"Source", {
                                            {"File", {}, [=](Registry* r, int id, int* fIdx){FileSelection(r, id, fIdx);}},
                                            {"Live", {}, [=](Registry* r, int id, int* fIdx){LiveInputSelection(r, id, fIdx);}},
                                            {"Shader", {}}
                                        }};
    m_menus[MT_PlaybackSelection]   = {"Playback", {}, [=](Registry* r, int id, int* fIdx){PlaybackSettings(r, id, fIdx);}};
    m_menus[MT_NetworkInfo]         = {"Network", {}, [=](Registry* r, int id, int* fIdx){NetworkInfo(r, id, fIdx);}};
    m_menus[MT_SettingsSelection]   = {"Settings", {}, [=](Registry* r, int id, int* fIdx){GlobalSettings(r, id, fIdx);}};
}

void MenuSystem::subscribeToEvents()
{
    // Examples:

    // Media Slot Event
    m_eventBus.subscribe<MediaSlotEvent>([](const MediaSlotEvent& event) {
        printf("Media Slot Event - (Slot Idx: %d)\n", event.slotId);
    });

    // Edit Mode Event
    m_eventBus.subscribe<EditModeEvent>([](const EditModeEvent& event) {
        printf("Edit Mode Event - (Mode Idx: %d)\n", event.modeId);
    });

    // Navigation Event
    m_eventBus.subscribe<NavigationEvent>([](const NavigationEvent& event) {
        printf("Navigation Event - (Type: %d)\n", (int)event.type);
    });
}

void MenuSystem::setMenu(MenuType menuType)
{
    if (m_menus.find(menuType) != m_menus.end())
    {
        m_currentMenuType = menuType;
        m_focusedIdx = 0;
    }
}

void MenuSystem::handleMediaAndEditButtons()
{
    int numButtons = m_keyboardShortcuts.size();

    // check the media-buttons
    for (int i = 0; i < numButtons; ++i)
    {
        ImGuiKey key = m_keyboardShortcuts[i];
        if (ImGui::IsKeyDown(key))
        {
            int bank = m_registry.inputMappings().bank;
            int id = bank * numButtons + i;
            m_id = id;

            if (m_currentMenuType == MT_StartupScreen) setMenu(MT_InputSelection);
            return;
        }
    }

    // check the edit-buttons
    for (ImGuiKey editButton : m_keyboardShortcuts_editButtons)
    {
        if (ImGui::IsKeyDown(editButton))
        {
            switch (editButton)
            {
            case ImGuiKey_Q:
                setMenu(MT_InfoSelection);
                break;
            case ImGuiKey_W:
                setMenu(MT_InputSelection);
                break;
            case ImGuiKey_E:
                setMenu(MT_PlaybackSelection);
                break;
            case ImGuiKey_R:
                break;
            case ImGuiKey_T:
                break;
            case ImGuiKey_Z:
                break;
            case ImGuiKey_U:
                setMenu(MT_NetworkInfo);
                break;
            case ImGuiKey_I:
                setMenu(MT_SettingsSelection);
                break;
            default:
                break;
            }

            
        }
    }
}

void MenuSystem::HandleUpAndDownKeys()
{
    // if (!ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
    //     m_ui.FocusPreviousElement();
    // }
    // else if (!ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
    //     m_ui.FocusNextElement();
    // }
}

void MenuSystem::render()
{
    m_ui.NewFrame();
    handleMediaAndEditButtons();

    // if (!m_currentMenu)
    // {
    //     m_ui.CenteredText("VM-1");
    //     return;
    // }

    // Traverse to current menu
    const MenuItem* menuItem = &(m_menus[m_currentMenuType]);
    for (int idx : m_currentMenuPath) {
        if (idx >= 0 && idx < (int)menuItem->children.size())
            menuItem = &menuItem->children[idx];
        else
            break;
    }

    // Render title
    if (!menuItem->label.empty()) {
        m_ui.MenuTitle(menuItem->label);
    }

    // Render bank information
    if (m_currentMenuType == MT_InfoSelection  || 
        m_currentMenuType == MT_InputSelection || 
        m_currentMenuType == MT_PlaybackSelection) {
            int id16 = m_id % 16;
            char bank = m_id / 16 + 65;
            std::string mediaSlotString = std::string(1, bank) + std::to_string(id16);
            m_ui.MenuInfo(mediaSlotString);
        }


    // Render dynamic content (if present)
    if (menuItem->func) {
        menuItem->func(&m_registry, m_id, &m_focusedIdx);
    }
    // Otherwise render static content
    else { 
        m_ui.BeginList(&m_focusedIdx);
        for (int i = 0; i < (int)menuItem->children.size(); ++i) {
            std::string label = menuItem->children[i].label;
            m_ui.Text(label);
        }
        m_ui.EndList();
    }

    HandleUpAndDownKeys();
    

    // Handle bank switching
    if (ImGui::IsKeyDown(ImGuiKey_LeftShift) && (ImGui::IsKeyPressed(ImGuiKey_RightArrow))) {
        m_registry.inputMappings().bank = (m_registry.inputMappings().bank + 1) % 4; 
    }
    else if (ImGui::IsKeyDown(ImGuiKey_LeftShift) && (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))) {
        m_registry.inputMappings().bank = (m_registry.inputMappings().bank - 1) % 4;
        if (m_registry.inputMappings().bank < 0) m_registry.inputMappings().bank = 3;
    }


    // Handle navigation
    if  ( (!ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_RightArrow)) || 
        (ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_DownArrow)) ) {
        // Go deeper if selected item has children or a render_func (treat as a page)
        if (m_focusedIdx >= 0 && m_focusedIdx < (int)menuItem->children.size()) {
            auto& sel = menuItem->children[m_focusedIdx];
            if (!sel.children.empty() || sel.func) {
                m_currentMenuPath.push_back(m_focusedIdx);
                m_focusedIdx = 0;
            }
        }
    }
    else if ( (!ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) || 
            (ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_UpArrow))) 
    {
        // Go up one level
        if (!m_currentMenuPath.empty()) {
            m_focusedIdx = m_currentMenuPath.back();
            m_currentMenuPath.pop_back();
        }
        else {
            //m_focusedIdx = 0;
            //setMenu(MT_InputSelection);
        }
    }
}

// DYNAMIC CONTENT
// TODO: Could be put in different namespaces
void MenuSystem::StartupScreen(Registry* registry, int id, int* focusedIdx)
{
    m_ui.CenteredText("VM-1");
    //m_ui.Text("VM-1");
}

void MenuSystem::FileSelection(Registry* registry, int id, int* focusedIdx)
{
    auto config = std::make_unique<VideoInputConfig>();
    config->looping = registry->settings().defaultLooping;

    VideoInputConfig* currentConfig = registry->inputMappings().getVideoInputConfig(id);
    if (currentConfig) {
        *config = *currentConfig;
    } 

    std::vector<std::string>& files = registry->mediaPool().getVideoFiles();
    bool changed = false;
    m_ui.BeginList(focusedIdx);
    for (int i = 0; i < files.size(); ++i) {
        std::string fileName = files[i];
        if (m_ui.RadioButton(fileName.c_str(), (config->fileName == fileName))) {
            config->fileName = fileName;
            changed = true;
        }
    }
    m_ui.EndList(); 

    if (changed)
        registry->inputMappings().addInputConfig(id, std::move(config));
}

void MenuSystem::LiveInputSelection(Registry* registry, int id, int* focusedIdx) 
{
    auto config = std::make_unique<HdmiInputConfig>();
    HdmiInputConfig* currentConfig = registry->inputMappings().getHdmiInputConfig(id);
    if (currentConfig) { *config = *currentConfig; }

    bool changed = false;
    m_ui.BeginList(focusedIdx);
    if (m_ui.RadioButton("HDMI 1", currentConfig && (config->hdmiPort == 0))) {
        config->hdmiPort = 0; 
        changed = true; 
    }
    if (m_ui.RadioButton("HDMI 2", currentConfig && (config->hdmiPort == 1))) { 
        config->hdmiPort = 1; 
        changed = true; 
    }
    m_ui.EndList();


    if (changed)  {
        //printf("ID: %d, PORT: %d\n", id, config->hdmiPort);
        registry->inputMappings().addInputConfig(id, std::move(config));
    }
}

void MenuSystem::PlaybackSettings(Registry* registry, int id, int* focusedIdx) 
{
    InputConfig* currentConfig = registry->inputMappings().getInputConfig(id);
    if (!currentConfig) {
        m_ui.BeginList(focusedIdx);
        m_ui.Text("No input selected");
        m_ui.EndList();
        return;
    }
    
    if (VideoInputConfig* videoInputConfig = dynamic_cast<VideoInputConfig*>(currentConfig)) {
        int i = 0;

        m_ui.BeginList(focusedIdx);
        if (m_ui.CheckBox("loop", videoInputConfig->looping)) { 
            videoInputConfig->looping = !videoInputConfig->looping; 
        }
        if (m_ui.CheckBox("backwards", videoInputConfig->backwards)) {
            videoInputConfig->backwards = !videoInputConfig->backwards;
        }
        m_ui.Text("start-time");
        m_ui.Text("end-time");
        m_ui.EndList();
    }
    else if (HdmiInputConfig* hdmiInputConfig = dynamic_cast<HdmiInputConfig*>(currentConfig)) {
        std::string label = "Source: HDMI " + hdmiInputConfig->hdmiPort;

        m_ui.BeginList(focusedIdx);
        m_ui.Text(label);
        m_ui.EndList();
    }
}

void MenuSystem::NetworkInfo(Registry* registry, int id, int* focusedIdx) 
{
    Settings& settings = registry->settings();
    std::string eth0;
    std::string wlan0;
    NetworkTools::getIPAddress("eth0", eth0);
    NetworkTools::getIPAddress("wlan0", wlan0);

    m_ui.BeginList(focusedIdx);
    if (!eth0.empty()) m_ui.Text("e: " + eth0);
    if (!wlan0.empty()) m_ui.Text("w: " + wlan0);
    m_ui.EndList();
}

void MenuSystem::GlobalSettings(Registry* registry, int id, int* focusedIdx) 
{
    Settings& settings = registry->settings();

    m_ui.BeginList(focusedIdx);
    m_ui.SpinBoxInt("Fade Time", settings.fadeTime, 0, 10);
    if (m_ui.CheckBox("Show UI", settings.showUI)) { settings.showUI = !settings.showUI; };
    if (m_ui.CheckBox("Default Looping", settings.defaultLooping)) { settings.defaultLooping = !settings.defaultLooping; };
    m_ui.EndList();
}