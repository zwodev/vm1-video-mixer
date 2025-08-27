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
    subscribeToEvents();
    createMenus();
    setMenu(MT_StartupScreen);
}

void MenuSystem::subscribeToEvents()
{
    m_eventBus.subscribe<PlaybackEvent>([this](const PlaybackEvent& event) {
        showPopupMessage(event.message);
    });
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
    m_menus[MT_PlaybackSelection]   = {"Play", {}, [this](int id, int* fIdx){PlaybackSettings(id, fIdx);}};
    m_menus[MT_NetworkInfo]         = {"Network", {}, [this](int id, int* fIdx){NetworkInfo(id, fIdx);}};
    m_menus[MT_SettingsSelection]   = {"Settings", {}, [this](int id, int* fIdx){GlobalSettings(id, fIdx);}};
    m_menus[MT_DeviceSettings]      = {"Devices", {}, [this](int id, int* fIdx){DeviceSettings(id, fIdx);}};
    m_menus[MT_ButtonMatrix]        = {"Keys", {}, [this](int id, int* fIdx){ButtonMatrix(id, fIdx);}};
    
}

void MenuSystem::setMenu(MenuType menuType)
{
    if (m_menus.find(menuType) != m_menus.end())
    {
        m_currentMenuType = menuType;
        m_focusedIdx = 0;
    }
}

void MenuSystem::showPopupMessage(const std::string& message) 
{
    m_lastPopupMessage = message;
    m_launchPopup = true;
}

void MenuSystem::handlePopupMessage()
{
    if (m_launchPopup && !m_lastPopupMessage.empty()) {
        m_ui.StartOverlay([this]() { m_ui.ShowPopupMessage(m_lastPopupMessage); });
        m_launchPopup = false;
    }
}

void MenuSystem::handleMediaAndEditButtons()
{
    // check the media-slot-ids
    for (int mediaSlotId : m_ui.getTriggeredMediaSlotIds())
    {
        m_id = mediaSlotId;

        
        if (m_currentMenuType == MT_StartupScreen) {
            m_eventBus.publish(EditModeEvent(1)); // event needs to be published to update DeviceController
            setMenu(MT_InputSelection); // todo: calling setMenu() here shouldn't be necessary. 
                                        // It should automatically be called in the for-loop below.
        }
        return;
    }

    // check the edit-buttons
    for (int editButtonId : m_ui.getTriggeredEditButtons())
    {
        std::cout << "editButtonId: " << editButtonId << std::endl;

        switch (editButtonId)
        {
        case 0:
            setMenu(MT_PlaybackSelection);
            break;
        case 1:
            setMenu(MT_ButtonMatrix);
            break;
        case 2:
            setMenu(MT_InputSelection);
            break;
        case 3:
            setMenu(MT_DeviceSettings);
            break;
        case 4:
            setMenu(MT_SettingsSelection);
            break;
        case 5:
            setMenu(MT_NetworkInfo);
            break;
        case 6:
            showPopupMessage("Not implemented");
            break;
        case 7:
            showPopupMessage("Not implemented");
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
        m_ui.StartOverlay([this](){m_ui.ShowBankInfo(m_registry.inputMappings().bank);});
    }
    else if (m_ui.isNavigationEventTriggered(NavigationEvent::Type::BankUp)) {
        m_registry.inputMappings().bank = (m_registry.inputMappings().bank - 1) % BANK_COUNT;
        if (m_registry.inputMappings().bank < 0) m_registry.inputMappings().bank = BANK_COUNT - 1;
        m_ui.StartOverlay([this](){m_ui.ShowBankInfo(m_registry.inputMappings().bank);});
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
        m_currentMenuType == MT_PlaybackSelection ||
        m_currentMenuType == MT_ButtonMatrix) 
    {
        int id16 = (m_id % MEDIA_BUTTON_COUNT) + 1;
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

    // Render Overlay
    m_ui.ShowOverlay();

    handlePopupMessage();
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
    m_ui.startUpLogo();
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
    
    int i = 0;
    m_ui.BeginList(focusedIdx);
    if (VideoInputConfig* videoInputConfig = dynamic_cast<VideoInputConfig*>(currentConfig)) {
        if (m_ui.CheckBox("loop", videoInputConfig->looping)) { 
            videoInputConfig->looping = !videoInputConfig->looping; 
        }
        std::string fileName = "Source: FILE ";
        m_ui.Text(fileName);
        // Not implemented yet
        // if (m_ui.CheckBox("backwards", videoInputConfig->backwards)) {
        //     videoInputConfig->backwards = !videoInputConfig->backwards;
        // }
        // m_ui.Text("start-time");
        // m_ui.Text("end-time");
    }
    else if (HdmiInputConfig* hdmiInputConfig = dynamic_cast<HdmiInputConfig*>(currentConfig)) {
        std::string inputName = "Source: HDMI" + std::to_string(hdmiInputConfig->hdmiPort+1);
        m_ui.Text(inputName);
    }
    if (m_ui.Action("Clear slot")) {
        m_registry.inputMappings().removeConfig(id);
    }
    m_ui.EndList();
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
    m_ui.Text("SSID: VM-1");
    m_ui.Text("Pass: vmone12345");
    m_ui.EndList();
    
    // const ImageBuffer& imageBuffer = m_registry.mediaPool().getQrCodeImageBuffer();
    // if (imageBuffer.isValid) {
    //     m_ui.Image(imageBuffer);
    // }
}

void MenuSystem::GlobalSettings(int id, int* focusedIdx) 
{
    Settings& settings = m_registry.settings();

    m_ui.BeginList(focusedIdx);
    m_ui.Text("Version: " + VERSION);
    m_ui.SpinBoxInt("Fade Time", settings.fadeTime, 0, 10);
    m_ui.SpinBoxInt("Volume", settings.volume, 0, 10);
    //if (m_registry.settings().isProVersion) { m_ui.SpinBoxInt("Rot. Sensit.", settings.rotarySensitivity, 1, 20) };
    if (m_ui.CheckBox("Show UI", settings.showUI)) { settings.showUI = !settings.showUI; };
    if (m_ui.CheckBox("Default Looping", settings.defaultLooping)) { settings.defaultLooping = !settings.defaultLooping; };
    m_ui.EndList();
}

void MenuSystem::DeviceSettings(int id, int* focusedIdx) 
{
    Settings& settings = m_registry.settings();

    m_ui.BeginList(focusedIdx);
    if (m_registry.settings().isHdmiOutputReady && m_registry.settings().isHdmiInputReady) {
        if (m_ui.Action("Scan for new")) {
            m_registry.settings().isHdmiOutputReady = false;
            m_registry.settings().isHdmiInputReady = false;
            m_eventBus.publish(SystemEvent(SystemEvent::Type::Restart));  
        }
    }
    else {
        m_ui.Text("Scanning...");
    }
    
    auto& hdmiOutputs = m_registry.settings().hdmiOutputs;
    for (size_t i = 0; i < hdmiOutputs.size(); ++i) {
        std::string displayConfig = !(hdmiOutputs[i].empty()) ? hdmiOutputs[i] : "Not connected";
        m_ui.Text(std::string("O") + std::to_string(i+1) + ": " + displayConfig);
    }

    auto& hdmiInputs = m_registry.settings().hdmiInputs;
    for (size_t i = 0; i < hdmiInputs.size(); ++i) {
        std::string inputConfig = !(hdmiInputs[i].empty()) ? hdmiInputs[i] : "Not connected";
        m_ui.Text(std::string("I") + std::to_string(i+1) + ": " + inputConfig);
    }

    m_ui.EndList();
}

void MenuSystem::ButtonMatrix(int id, int* focusedIdx) 
{
    int bank = m_registry.inputMappings().bank;
    for (int i = 0; i < m_buttonTexts.size(); ++i) {
        int mediaSlot = bank * 16 + i;
        InputConfig* currentConfig = m_registry.inputMappings().getInputConfig(mediaSlot);
        if (!currentConfig) {
            m_buttonTexts[i].second = COLOR::BLACK;
        }

        if (VideoInputConfig* videoInputConfig = dynamic_cast<VideoInputConfig*>(currentConfig)) {
            if (videoInputConfig->isActive) m_buttonTexts[i].second = COLOR::RED;
            else m_buttonTexts[i].second = COLOR::DARK_RED;
        }
        else if (HdmiInputConfig* hdmiInputConfig = dynamic_cast<HdmiInputConfig*>(currentConfig)) {
            if (hdmiInputConfig->isActive) m_buttonTexts[i].second = COLOR::BLUE;
            else m_buttonTexts[i].second = COLOR::DARK_BLUE;
        }
    }
    m_ui.ShowButtonMatrix(m_buttonTexts);
}