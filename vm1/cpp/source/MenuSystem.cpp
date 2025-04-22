/*
 * Copyright (c) 2025 Julian Jungel & Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include "MenuSystem.h"
#include "UIHelper.h"

#include <imgui.h>
#include <vector>
#include <string>

const int NUM_BANKS = 4;

MenuSystem::MenuSystem(Registry &registry) : m_registry(registry)
{   
    m_menus[MT_StartupScreen]       =  {"", {}, StartupScreen};
    m_menus[MT_InfoSelection]       =  {"Info", {}};
    m_menus[MT_InputSelection]      =  {"Source", {
                                            {"File", {}, FileSelection},
                                            {"Live", {}, LiveInputSelection},
                                            {"Shader", {}}
                                        }};
    m_menus[MT_PlaybackSelection]   = {"Playback", {}, PlaybackSettings};
    m_menus[MT_SettingsSelection]   = {"Settings", {}, GlobalSettings};

    setMenu(MT_StartupScreen);
}

void MenuSystem::setMenu(MenuType menuType)
{
    if (m_menus.find(menuType) != m_menus.end())
    {
        m_currentMenuType = menuType;
        m_selectedIdx = 0;
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
            int id = m_bank * numButtons + i;
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

void MenuSystem::HandleUpAndDownKeys(int* selectedIdx, int menuSize)
{
    if (!ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        if ((*selectedIdx) > 0) {
            (*selectedIdx)--;
            UI::resetTextScrollPosition();
        }
    }
    else if (!ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        if ((*selectedIdx) + 1 < menuSize) {
            (*selectedIdx)++;
            UI::resetTextScrollPosition();
        }
    }
}

void MenuSystem::render()
{
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
        UI::MediaButtonID(m_id + 1);
    }

    ImGui::SetCursorPosY(23);

    // Render static content
    for (int i = 0; i < (int)menuItem->children.size(); ++i) {
        bool isSelected = (i == m_selectedIdx);
        std::string label = menuItem->children[i].label;
        UI::Text(label, isSelected ? UI::TextState::SELECTED : UI::TextState::DEFAULT);
    }

    // Render dynamic content (if present)
    if (menuItem->renderFunc) {
        menuItem->renderFunc(&m_registry, m_id, &m_selectedIdx);
    }

    // Handle navigation
    if  (ImGui::IsKeyPressed(ImGuiKey_RightArrow) || 
        (ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_DownArrow))) {
        // Go deeper if selected item has children or a render_func (treat as a page)
        if (m_selectedIdx >= 0 && m_selectedIdx < (int)menuItem->children.size()) {
            auto& sel = menuItem->children[m_selectedIdx];
            if (!sel.children.empty() || sel.renderFunc) {
                m_currentMenuPath.push_back(m_selectedIdx);
                m_selectedIdx = 0;
            }
        }
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || 
            (ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_UpArrow))) 
    {
        // Go up one level
        if (!m_currentMenuPath.empty()) {
            m_selectedIdx = m_currentMenuPath.back();
            m_currentMenuPath.pop_back();
        }
        else {
            m_selectedIdx = 0;
            setMenu(MT_InputSelection);
        }
    }
    else if (menuItem->children.size() > 0) {
        HandleUpAndDownKeys(&m_selectedIdx, (int)menuItem->children.size());
    }

}

// DYNAMIC CONTENT
// TODO: Could be put in different namespaces
void MenuSystem::StartupScreen(Registry* registry, int id, int* selectedIdx)
{
    UI::CenteredText("VM-1");
}

void MenuSystem::FileSelection(Registry* registry, int id, int* selectedIdx)
{
    auto config = std::make_unique<VideoInputConfig>();
    VideoInputConfig* currentConfig = registry->inputMappings().getVideoInputConfig(id);
    if (currentConfig) {
        *config = *currentConfig;
    } 

    std::vector<std::string>& files = registry->mediaPool().getVideoFiles();
    bool changed = false;
    int menuSize = 0;
    for (int i = 0; i < files.size(); ++i) {
        std::string fileName = files[i];
        if (UI::RadioButton(fileName.c_str(), *selectedIdx == i, (config->fileName == fileName))) {
            config->fileName = fileName;
            changed = true;
        }
        menuSize++;
    }

    if (changed)
        registry->inputMappings().addInputConfig(id, std::move(config));

    HandleUpAndDownKeys(selectedIdx, menuSize);   
}

void MenuSystem::LiveInputSelection(Registry* registry, int id, int* selectedIdx) 
{
    auto config = std::make_unique<HdmiInputConfig>();
    HdmiInputConfig* currentConfig = registry->inputMappings().getHdmiInputConfig(id);
    if (currentConfig) { *config = *currentConfig; }

    int i = 0;
    bool changed = false;
    if (UI::RadioButton("HDMI 1", (i++ == *selectedIdx), currentConfig && (config->hdmiPort == 0))) {
        config->hdmiPort = 0; 
        changed = true; 
    }
    if (UI::RadioButton("HDMI 2", (i++ == *selectedIdx), currentConfig && (config->hdmiPort == 1))) { 
        config->hdmiPort = 1; 
        changed = true; 
    }
    if (changed) registry->inputMappings().addInputConfig(id, std::move(config));

    HandleUpAndDownKeys(selectedIdx, i);

}

void MenuSystem::PlaybackSettings(Registry* registry, int id, int* selectedIdx) 
{
    InputConfig* currentConfig = registry->inputMappings().getInputConfig(id);
    if (!currentConfig) {
        UI::Text("No input selected", UI::TextState::DEFAULT);
        return;
    }
    
    if (VideoInputConfig* videoInputConfig = dynamic_cast<VideoInputConfig*>(currentConfig)) {
        int i = 0;
        if (UI::CheckBox("loop", (i++ == *selectedIdx), videoInputConfig->looping)) { 
            videoInputConfig->looping = !videoInputConfig->looping; 
        }
        if (UI::CheckBox("backwards", (i++ == *selectedIdx), videoInputConfig->backwards)) {
            videoInputConfig->backwards = !videoInputConfig->backwards;
        }
        UI::Text("start-time", (i++ == *selectedIdx) ? UI::TextState::SELECTED : UI::TextState::DEFAULT);
        UI::Text("end-time", (i++ == *selectedIdx) ? UI::TextState::SELECTED : UI::TextState::DEFAULT);
        HandleUpAndDownKeys(selectedIdx, i);
    }
    else if (HdmiInputConfig* hdmiInputConfig = dynamic_cast<HdmiInputConfig*>(currentConfig)) {
        std::string label = "Source: HDMI " + hdmiInputConfig->hdmiPort;
        UI::Text(label, UI::TextState::DEFAULT);
    }
}

void MenuSystem::GlobalSettings(Registry* registry, int id, int* selectedIdx) 
{
    Settings& settings = registry->settings();

    int i = 0;
    if (UI::CheckBox("Show UI", (i++ == *selectedIdx), settings.showUI)) { settings.showUI = !settings.showUI; };
    HandleUpAndDownKeys(selectedIdx, i);
}