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
#include "VM1DeviceDefinitions.h"

#include <imgui.h>
#include <vector>
#include <string>

const int NUM_BANKS = 4;

MenuSystem::MenuSystem(UI& ui, Registry& registry, EventBus& eventBus) : 
    m_ui(ui), 
    m_registry(registry), 
    m_eventBus(eventBus)
{   
    createMenus();
    setMenu(MT_StartupScreen);
}

void MenuSystem::createMenus()
{
    m_menus[MT_StartupScreen]       =  {"", {}, [this](int id, int* fIdx){StartupScreen(id, fIdx);}};
    m_menus[MT_InfoSelection]       =  {"Info", {}};
    m_menus[MT_InputSelection]      =  {"Source", {
                                            {"File", {}, [this](int id, int* fIdx){FileSelection(id, fIdx);}},
                                            {"Live", {}, [this](int id, int* fIdx){LiveInputSelection(id, fIdx);}},
                                            {"Shader", {}}
                                        }};
    m_menus[MT_PlaybackSelection]   = {"Playback", {}, [this](int id, int* fIdx){PlaybackSettings(id, fIdx);}};
    m_menus[MT_NetworkInfo]         = {"Network", {}, [this](int id, int* fIdx){NetworkInfo(id, fIdx);}};
    m_menus[MT_SettingsSelection]   = {"Settings", {}, [this](int id, int* fIdx){GlobalSettings(id, fIdx);}};
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
    // check the media-slot-ids
    for (int mediaSlotId : m_ui.getTriggeredMediaSlotIds())
    {
        m_id = mediaSlotId;

        if (m_currentMenuType == MT_StartupScreen) setMenu(MT_InputSelection);
        return;
    }

    // check the edit-buttons
    for (int editButtonId : m_ui.getTriggeredEditButtons())
    {
        switch (editButtonId)
        {
        case 0:
            setMenu(MT_InfoSelection);
            break;
        case 1:
            setMenu(MT_InputSelection);
            break;
        case 2:
            setMenu(MT_PlaybackSelection);
            break;
        case 3:
            break;
        case 4:
            break;
        case 5:
            break;
        case 6:
            setMenu(MT_NetworkInfo);
            break;
        case 7:
            setMenu(MT_SettingsSelection);
            break;
        default:
            break;
        }
    }
}

void MenuSystem::handleUpAndDownKeys()
{
    if(m_ui.isNavigationEventTriggered(NavigationEvent::Type::FocusNext))
    {
        m_ui.FocusNextElement();
    }
    else if(m_ui.isNavigationEventTriggered(NavigationEvent::Type::FocusPrevious))
    {
        m_ui.FocusPreviousElement();
    }
}

void MenuSystem::handleBankSwitching()
{
    if (m_ui.isNavigationEventTriggered(NavigationEvent::Type::BankDown)) {
        m_registry.inputMappings().bank = (m_registry.inputMappings().bank + 1) % BANK_COUNT; 
    }
    else if (m_ui.isNavigationEventTriggered(NavigationEvent::Type::BankUp)) {
        m_registry.inputMappings().bank = (m_registry.inputMappings().bank - 1) % BANK_COUNT;
        if (m_registry.inputMappings().bank < 0) m_registry.inputMappings().bank = BANK_COUNT - 1;
    }
}

void MenuSystem::handleMenuHierachyNavigation(const MenuItem *menuItem)
{
    // Handle navigation
    if  (m_ui.isNavigationEventTriggered(NavigationEvent::Type::HierarchyDown)) {
        // Go deeper if selected item has children or a render_func (treat as a page)
        if (m_focusedIdx >= 0 && m_focusedIdx < (int)menuItem->children.size()) {
            auto& sel = menuItem->children[m_focusedIdx];
            if (!sel.children.empty() || sel.func) {
                m_currentMenuPath.push_back(m_focusedIdx);
                m_focusedIdx = 0;
            }
        }
    }
    else if (m_ui.isNavigationEventTriggered(NavigationEvent::Type::HierarchyUp)) 
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

void MenuSystem::render()
{
    m_ui.NewFrame();

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
        m_currentMenuType == MT_PlaybackSelection) 
    {
        int id16 = m_id % MEDIA_BUTTON_COUNT;
        char bank = m_id / MEDIA_BUTTON_COUNT + 65; // "+65" to get ASCII code
        std::string mediaSlotString = std::string(1, bank) + std::to_string(id16);

        m_ui.MenuInfo(mediaSlotString);
    }


    // Render dynamic content (if present)
    if (menuItem->func) 
    {
        menuItem->func(m_id, &m_focusedIdx);
    }
    // Otherwise render static content
    else 
    { 
        m_ui.BeginList(&m_focusedIdx);
        for (int i = 0; i < (int)menuItem->children.size(); ++i) {
            std::string label = menuItem->children[i].label;
            m_ui.Text(label);
        }
        m_ui.EndList();
    }

    handleMediaAndEditButtons();
    handleUpAndDownKeys();    
    handleBankSwitching();
    handleMenuHierachyNavigation(menuItem);


    m_ui.EndFrame();
}

// DYNAMIC CONTENT
// TODO: Could be put in different namespaces
void MenuSystem::StartupScreen(int id, int* focusedIdx)
{
    m_ui.CenteredText("VM-1");
    //m_ui.Text("VM-1");
}

void MenuSystem::FileSelection(int id, int* focusedIdx)
{
    auto config = std::make_unique<VideoInputConfig>();
    config->looping = m_registry.settings().defaultLooping;

    VideoInputConfig* currentConfig = m_registry.inputMappings().getVideoInputConfig(id);
    if (currentConfig) {
        *config = *currentConfig;
    } 

    std::vector<std::string>& files = m_registry.mediaPool().getVideoFiles();
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
        m_registry.inputMappings().addInputConfig(id, std::move(config));
}

void MenuSystem::LiveInputSelection(int id, int* focusedIdx) 
{
    auto config = std::make_unique<HdmiInputConfig>();
    HdmiInputConfig* currentConfig = m_registry.inputMappings().getHdmiInputConfig(id);
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
        m_registry.inputMappings().addInputConfig(id, std::move(config));
    }
}

void MenuSystem::PlaybackSettings(int id, int* focusedIdx) 
{
    InputConfig* currentConfig = m_registry.inputMappings().getInputConfig(id);
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

void MenuSystem::NetworkInfo(int id, int* focusedIdx) 
{
    Settings& settings = m_registry.settings();
    std::string eth0;
    std::string wlan0;
    NetworkTools::getIPAddress("eth0", eth0);
    NetworkTools::getIPAddress("wlan0", wlan0);

    m_ui.BeginList(focusedIdx);
    if (!eth0.empty()) m_ui.Text("e: " + eth0);
    if (!wlan0.empty()) m_ui.Text("w: " + wlan0);
    m_ui.EndList();
}

void MenuSystem::GlobalSettings(int id, int* focusedIdx) 
{
    Settings& settings = m_registry.settings();

    m_ui.BeginList(focusedIdx);
    m_ui.SpinBoxInt("Fade Time", settings.fadeTime, 0, 10);
    if (m_ui.CheckBox("Show UI", settings.showUI)) { settings.showUI = !settings.showUI; };
    if (m_ui.CheckBox("Default Looping", settings.defaultLooping)) { settings.defaultLooping = !settings.defaultLooping; };
    m_ui.EndList();
}