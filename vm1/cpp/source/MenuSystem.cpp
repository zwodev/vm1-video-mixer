/*
 * Copyright (c) 2025 Julian Jungel & Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include "MenuSystem.h"
#include "UIHelper.h"
#include "NetworkTools.h"

#include <imgui.h>
#include <vector>
#include <string>

const int NUM_BANKS = 4;

MenuSystem::MenuSystem(Registry& registry, EventBus& eventBus) : m_registry(registry), m_eventBus(eventBus)
{   
    createMenus();
    subscribeToEvents();
    setMenu(MT_StartupScreen);
}

void MenuSystem::createMenus()
{
    m_menus[MT_StartupScreen]       =  {"", {}, StartupScreen};
    m_menus[MT_InfoSelection]       =  {"Info", {}};
    m_menus[MT_InputSelection]      =  {"Source", {
                                            {"File", {}, FileSelection},
                                            {"Live", {}, LiveInputSelection},
                                            {"Shader", {}}
                                        }};
    m_menus[MT_PlaybackSelection]   = {"Playback", {}, PlaybackSettings};
    m_menus[MT_NetworkInfo]         = {"Network", {}, NetworkInfo};
    m_menus[MT_SettingsSelection]   = {"Settings", {}, GlobalSettings};
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
    if (!ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        UI::FocusPreviousElement();
    }
    else if (!ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        UI::FocusNextElement();
    }
}

void MenuSystem::render()
{
    UI::NewFrame();
    handleMediaAndEditButtons();

    // if (!m_currentMenu)
    // {
    //     UI::CenteredText("VM-1");
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
        UI::MenuTitle(menuItem->label);
    }

    // Render bank information
    if (m_currentMenuType == MT_InfoSelection  || 
        m_currentMenuType == MT_InputSelection || 
        m_currentMenuType == MT_PlaybackSelection) {
            int id16 = m_id % 16;
            char bank = m_id / 16 + 65;
            std::string mediaSlotString = std::string(1, bank) + std::to_string(id16);
            UI::MenuInfo(mediaSlotString);
        }


    // Render dynamic content (if present)
    if (menuItem->renderFunc) {
        menuItem->renderFunc(&m_registry, m_id, &m_focusedIdx);
    }
    // Otherwise render static content
    else { 
        UI::BeginList(&m_focusedIdx);
        for (int i = 0; i < (int)menuItem->children.size(); ++i) {
            std::string label = menuItem->children[i].label;
            UI::Text(label);
        }
        UI::EndList();
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
            if (!sel.children.empty() || sel.renderFunc) {
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
    UI::CenteredText("VM-1");
    //UI::Text("VM-1");
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
    UI::BeginList(focusedIdx);
    for (int i = 0; i < files.size(); ++i) {
        std::string fileName = files[i];
        if (UI::RadioButton(fileName.c_str(), (config->fileName == fileName))) {
            config->fileName = fileName;
            changed = true;
        }
    }
    UI::EndList(); 

    if (changed)
        registry->inputMappings().addInputConfig(id, std::move(config));
}

void MenuSystem::LiveInputSelection(Registry* registry, int id, int* focusedIdx) 
{
    auto config = std::make_unique<HdmiInputConfig>();
    HdmiInputConfig* currentConfig = registry->inputMappings().getHdmiInputConfig(id);
    if (currentConfig) { *config = *currentConfig; }

    bool changed = false;
    UI::BeginList(focusedIdx);
    if (UI::RadioButton("HDMI 1", currentConfig && (config->hdmiPort == 0))) {
        config->hdmiPort = 0; 
        changed = true; 
    }
    if (UI::RadioButton("HDMI 2", currentConfig && (config->hdmiPort == 1))) { 
        config->hdmiPort = 1; 
        changed = true; 
    }
    UI::EndList();


    if (changed)  {
        //printf("ID: %d, PORT: %d\n", id, config->hdmiPort);
        registry->inputMappings().addInputConfig(id, std::move(config));
    }
}

void MenuSystem::PlaybackSettings(Registry* registry, int id, int* focusedIdx) 
{
    InputConfig* currentConfig = registry->inputMappings().getInputConfig(id);
    if (!currentConfig) {
        UI::BeginList(focusedIdx);
        UI::Text("No input selected");
        UI::EndList();
        return;
    }
    
    if (VideoInputConfig* videoInputConfig = dynamic_cast<VideoInputConfig*>(currentConfig)) {
        int i = 0;

        UI::BeginList(focusedIdx);
        if (UI::CheckBox("loop", videoInputConfig->looping)) { 
            videoInputConfig->looping = !videoInputConfig->looping; 
        }
        if (UI::CheckBox("backwards", videoInputConfig->backwards)) {
            videoInputConfig->backwards = !videoInputConfig->backwards;
        }
        UI::Text("start-time");
        UI::Text("end-time");
        UI::EndList();
    }
    else if (HdmiInputConfig* hdmiInputConfig = dynamic_cast<HdmiInputConfig*>(currentConfig)) {
        std::string label = "Source: HDMI " + hdmiInputConfig->hdmiPort;

        UI::BeginList(focusedIdx);
        UI::Text(label);
        UI::EndList();
    }
}

void MenuSystem::NetworkInfo(Registry* registry, int id, int* focusedIdx) 
{
    Settings& settings = registry->settings();
    std::string eth0;
    std::string wlan0;
    NetworkTools::getIPAddress("eth0", eth0);
    NetworkTools::getIPAddress("wlan0", wlan0);

    UI::BeginList(focusedIdx);
    if (!eth0.empty()) UI::Text("e: " + eth0);
    if (!wlan0.empty()) UI::Text("w: " + wlan0);
    UI::EndList();
}

void MenuSystem::GlobalSettings(Registry* registry, int id, int* focusedIdx) 
{
    Settings& settings = registry->settings();

    UI::BeginList(focusedIdx);
    UI::SpinBoxInt("Fade Time", settings.fadeTime, 0, 10);
    if (UI::CheckBox("Show UI", settings.showUI)) { settings.showUI = !settings.showUI; };
    if (UI::CheckBox("Default Looping", settings.defaultLooping)) { settings.defaultLooping = !settings.defaultLooping; };
    UI::EndList();
}