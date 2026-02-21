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
#include "ImageBuffer.h"

#include <imgui.h>
#include <vector>
#include <string>

// const int NUM_BANKS = 4;

MenuSystem::MenuSystem(UI& ui, Registry& registry, EventBus& eventBus) : 
    m_ui(ui), 
    m_registry(registry), 
    m_eventBus(eventBus)
{   
    subscribeToEvents();
    createMenus();
    setMenu(MT_StartupScreen);
}

void MenuSystem::reset()
{
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
    m_menus[MT_InfoSelection]       =  {"Info", {}, [this](int id, int* fIdx){InfoScreen(id, fIdx);}};
    // m_menus[MT_InputSelection]      =  {"Source", {
    //                                         {"File", {}, [this](int id, int* fIdx){FileSelection(id, fIdx);}},
    //                                         {"Live", {}, [this](int id, int* fIdx){LiveInputSelection(id, fIdx);}},
    //                                         {"Shader", {}, [this](int id, int* fIdx){ShaderSelection(id, fIdx);}}
    //                                     }};
    m_menus[MT_InputSelection]      = {"Source", {}, [this](int id, int* fIdx){InputSelection(id, fIdx);}};
    m_menus[MT_MediaFiles]          = {"Media Files", {}, [this](int id, int* fIdx){FileSelection(id, fIdx);}};
    m_menus[MT_LiveInputs]          = {"HDMI Inputs", {}, [this](int id, int* fIdx){LiveInputSelection(id, fIdx);}};
    m_menus[MT_Shaders]             = {"Shaders", {}, [this](int id, int* fIdx){ShaderSelection(id, fIdx);}};

    m_menus[MT_Effects]             = {"Effects", {}, [this](int id, int* fIdx){Effects(id, fIdx);}};
    m_menus[MT_CustomFx]            = {"CustomFx", {}, [this](int id, int* fIdx){CustomFx(id, fIdx);}};
    m_menus[MT_ChromaKey]           = {"ChromaKey", {}, [this](int id, int* fIdx){ChromaKey(id, fIdx);}};
    m_menus[MT_ColorCorrection]     = {"ColorCorrection", {}, [this](int id, int* fIdx){ColorCorrection(id, fIdx);}};
    m_menus[MT_BlendMode]           = {"BlendMode", {}, [this](int id, int* fIdx){BlendMode(id, fIdx);}};

    m_menus[MT_Outputs]             = {"Outputs", {}, [this](int id, int* fIdx){OutputPlanes(id, fIdx);}};
    m_menus[MT_PlaneSettings]       = {"Plane Settings", {}, [this](int id, int* fIdx){PlaneSettings(id, fIdx);}};
    m_menus[MT_HdmiSelection]       = {"Hdmi Selection", {}, [this](int id, int* fIdx){HdmiSelection(id, fIdx);}};
    m_menus[MT_Mask]                = {"Mask", {}, [this](int id, int* fIdx){Mask(id, fIdx);}};
    m_menus[MT_Mapping]             = {"Mapping", {}, [this](int id, int* fIdx){Mapping(id, fIdx);}};

    m_menus[MT_PlaybackSelection]   = {"Playback/Control", {}, [this](int id, int* fIdx){PlaybackSettings(id, fIdx);}};
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
            //m_eventBus.publish(EditModeEvent(1)); // event needs to be published to update DeviceController
            //setMenu(MT_InputSelection); // todo: calling setMenu() here shouldn't be necessary. 
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
            // if(m_registry.settings().displayType == DisplayType::SSD1351_OLED) { 
            //     setMenu(MT_PlaybackSelection);
            // } else if(m_registry.settings().displayType == DisplayType::ILI9341_IPS_LCD) {
            //     setMenu(MT_InfoSelection);
            // }
            setMenu(MT_InfoSelection);
            break;
        case 1:
            setMenu(MT_InputSelection);
            break;
        case 2:
            setMenu(MT_PlaybackSelection);
            break;
        case 3:
            setMenu(MT_Effects);
            break;
        case 4:
            setMenu(MT_Outputs);
            break;
        case 5:
            setMenu(MT_DeviceSettings);
            break;
        case 6:
            setMenu(MT_SettingsSelection);
            break;
        case 7:
            if(!m_registry.settings().kiosk.enabled) {
                setMenu(MT_NetworkInfo);
            } else {
                showPopupMessage("Not implemented");
            }
            break;
        case 8:     // 8-15: Edit Keys with Fn-Button pressed
            setMenu(MT_ButtonMatrix);
            break;
        case 15:
            setMenu(MT_StartupScreen);
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

        // manually traverse backwards through UI hierachy:
        if(
            m_currentMenuType == MT_MediaFiles ||
            m_currentMenuType == MT_LiveInputs ||
            m_currentMenuType == MT_Shaders
        ){
            setMenu(MT_InputSelection);
        } 
        else if (
            m_currentMenuType == MT_CustomFx ||
            m_currentMenuType == MT_ChromaKey ||
            m_currentMenuType == MT_ColorCorrection ||
            m_currentMenuType == MT_BlendMode
        ){
            setMenu(MT_Effects);
        }
        else if (
            m_currentMenuType == MT_HdmiSelection ||
            m_currentMenuType == MT_Mask ||
            m_currentMenuType == MT_Mapping
        ){
            setMenu(MT_PlaneSettings);
        }
        else if (
            m_currentMenuType == MT_PlaneSettings
        ){
            setMenu(MT_Outputs);
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
    // update: render menu title in the menu-specific functions
    // if (!menuItem->label.empty()) {
    //     m_ui.MenuTitle(menuItem->label);
    // }

    // Render bank information
    if (m_currentMenuType == MT_InfoSelection  || 

        m_currentMenuType == MT_InputSelection || 
        m_currentMenuType == MT_MediaFiles || 
        m_currentMenuType == MT_LiveInputs || 
        m_currentMenuType == MT_Shaders || 

        m_currentMenuType == MT_Effects ||
        m_currentMenuType == MT_CustomFx ||
        m_currentMenuType == MT_ChromaKey ||
        m_currentMenuType == MT_ColorCorrection ||
        m_currentMenuType == MT_BlendMode ||

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
    if(m_registry.settings().displayType == DisplayType::SSD1351_OLED) { 
        m_ui.startUpLogo();
    } else if(m_registry.settings().displayType == DisplayType::ILI9341_IPS_LCD) {
        m_ui.Image(ImageBuffer("media/splash-screen.png"));
    }
}

void MenuSystem::InfoScreen(int id, int* focusedIdx)
{
    m_ui.MenuTitle("INFO");
    if(m_registry.settings().displayType == DisplayType::SSD1351_OLED) { 
      
    } else if(m_registry.settings().displayType == DisplayType::ILI9341_IPS_LCD) {
        // m_ui.Image(ImageBuffer("media/01-info.png"));
        
        m_ui.BeginList(focusedIdx);
        InputConfig *inputConfig = m_registry.inputMappings().getInputConfig(id);
        if (VideoInputConfig *videoInputConfig = dynamic_cast<VideoInputConfig *>(inputConfig))
        {
            m_ui.PlainText("Source:    Media File");
            m_ui.PlainText("Filename:  " + videoInputConfig->fileName);
            m_ui.PlainText("Duration:  ");
            m_ui.PlainText("In-Point:  ");
            m_ui.PlainText("Out-Point: ");
            m_ui.PlainText("Current Position:");
        }
        else if (HdmiInputConfig*hdmiInputConfig = dynamic_cast<HdmiInputConfig *>(inputConfig))
        {
            m_ui.PlainText("Source:     HDMI");
            m_ui.PlainText("Input Port: " + std::to_string(hdmiInputConfig->hdmiPort));
            m_ui.PlainText("Resolution: ");
        }
        else if (ShaderInputConfig*shaderInputConfig = dynamic_cast<ShaderInputConfig *>(inputConfig))
        {
            m_ui.PlainText("Source:     Shader");
            m_ui.PlainText("Filename:   " + shaderInputConfig->fileName);
            m_ui.PlainText("Parameters: ");
        }
        
        m_ui.EndList(); 
    }
}

void MenuSystem::InputSelection(int id, int* focusedIdx)
{
    m_ui.MenuTitle("SRC");

    m_ui.BeginList(focusedIdx);
    if(m_ui.Action("Files")){
        setMenu(MT_MediaFiles);
    } else if (m_ui.Action("HDMI Input")){
        setMenu(MT_LiveInputs);
    } else if (m_ui.Action("Shaders")) {
        setMenu(MT_Shaders);
    }
    m_ui.Break();
    if (m_ui.Action("Clear slot")) {
        m_registry.inputMappings().removeConfig(id);
    }
    m_ui.EndList();
}

void MenuSystem::FileSelection(int id, int* focusedIdx)
{
    m_ui.MenuTitle("SRC -> Media Files");
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
    m_ui.MenuTitle("SRC -> HDMI Inputs");
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

void MenuSystem::ShaderSelection(int id, int* focusedIdx)
{
    m_ui.MenuTitle("SRC -> Shader");

    auto config = std::make_unique<ShaderInputConfig>();

    ShaderInputConfig* currentConfig = m_registry.inputMappings().getShaderInputConfig(id);
    if (currentConfig) {
        *config = *currentConfig;
    } 

    std::vector<std::string>& files = m_registry.mediaPool().getShaderFiles();
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
    
    // auto config = std::make_unique<ShaderInputConfig>();
    // // config->looping = m_registry.settings().defaultLooping;

    // ShaderInputConfig* currentConfig = m_registry.inputMappings().getShaderInputConfig(id);
    // if (currentConfig) {
    //     *config = *currentConfig;
    // } 

    // std::vector<std::string>& files = m_registry.mediaPool().getVideoFiles();
    // bool changed = false;
    // m_ui.BeginList(focusedIdx);
    // if (m_ui.RadioButton("customShader", (config->fileName == "customShader"))) {
    //     config->fileName = "customShader";
    //     changed = true;
    // }

    // m_ui.EndList(); 

    // if (changed)
    //     m_registry.inputMappings().addInputConfig(id, std::move(config));
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
        m_ui.MenuTitle("CTRL Mediafile");
        m_ui.BeginList(focusedIdx);
        if (m_ui.CheckBox("loop", videoInputConfig->looping)) { 
            videoInputConfig->looping = !videoInputConfig->looping; 
        }
        if (m_ui.CheckBox("backwards", videoInputConfig->backwards)) {
            videoInputConfig->backwards = !videoInputConfig->backwards;
        }
        m_ui.Text("start-time");
        m_ui.Text("end-time");
        m_ui.Text("speed");
    }
    else if (HdmiInputConfig* hdmiInputConfig = dynamic_cast<HdmiInputConfig*>(currentConfig)) {
        m_ui.MenuTitle("CTRL Camera");        
        m_ui.BeginList(focusedIdx);
        std::string inputName = "Source: HDMI" + std::to_string(hdmiInputConfig->hdmiPort+1);
        m_ui.Text(inputName);
    }
    else if (dynamic_cast<ShaderInputConfig*>(currentConfig)) {
        m_ui.MenuTitle("CTRL Shader");        
        m_ui.BeginList(focusedIdx);
        m_ui.Text("Parameter 1");
        m_ui.Text("Parameter 2");
        m_ui.Text("Parameter 3");
    }
    m_ui.Break();
    int p = 1;
    m_ui.SpinBoxInt("Out Plane", p, 1, 4);
    m_ui.EndList();

    // int i = 0;
    // if (VideoInputConfig* videoInputConfig = dynamic_cast<VideoInputConfig*>(currentConfig)) {
    //     // Not implemented yet
    // }
    // else if (HdmiInputConfig* hdmiInputConfig = dynamic_cast<HdmiInputConfig*>(currentConfig)) {
    // }
}

void MenuSystem::Effects(int id, int* selectedIdx)
{
    m_ui.MenuTitle("FX");
    m_ui.BeginList(selectedIdx);
    if(m_ui.Action("[ ] Chroma Key")){
        setMenu(MT_ChromaKey);
    } else if (m_ui.Action("[ ] Custom Effect")){
        setMenu(MT_CustomFx);
    } else if (m_ui.Action("[x] Color Correction")) {
        setMenu(MT_ColorCorrection);
    } else if (m_ui.Action("[x] Blend Mode")) {
        setMenu(MT_BlendMode);
    }
    m_ui.EndList();

}

void MenuSystem::CustomFx(int id, int* selectedIdx)
{
    m_ui.MenuTitle("FX -> Custom FX");
    m_ui.BeginList(selectedIdx);
    m_ui.Text("If no fx is selected, file-list is visible.");
    m_ui.Text("If fx is selected, parameters are visible,");
    m_ui.Break();
    m_ui.RadioButton("file-1.frag", false);
    m_ui.RadioButton("file-2.frag", false);
    m_ui.RadioButton("file-3.frag", false);
    m_ui.RadioButton("file-4.frag", false);
    m_ui.Break();
    m_ui.Action("Reset");
    m_ui.Action("Remove (only visible when fx-shader active)");
    m_ui.CheckBox("Enabled", true);
    m_ui.EndList();
}

void MenuSystem::ChromaKey(int id, int* selectedIdx)
{
    m_ui.MenuTitle("FX -> ChromaKey");
    m_ui.BeginList(selectedIdx);
    m_ui.Text("Color");
    int tolerance = 50, smoothness = 50, spill = 50;
    m_ui.SpinBoxInt("Tolerance", tolerance, 0, 100);
    m_ui.SpinBoxInt("Smoothness", smoothness, 0, 100);
    m_ui.SpinBoxInt("Spill", spill, 0, 100);
    m_ui.Text("Pre-Mask");
    m_ui.Break();
    m_ui.Action("Reset");
    m_ui.CheckBox("Enabled", true);
    m_ui.EndList();
}

void MenuSystem::ColorCorrection(int id, int* selectedIdx)
{
    m_ui.MenuTitle("FX -> Color");
    m_ui.BeginList(selectedIdx);
    int black = 0, white = 100, gamma = 50, temperature = 50, tint = 50, sat = 50;
    m_ui.SpinBoxInt("Black-Point", black, 0, 100);
    m_ui.SpinBoxInt("White-Point", white, 0, 100);
    m_ui.SpinBoxInt("Gamma", gamma, 0, 100);
    m_ui.SpinBoxInt("Temperature", temperature, 0, 100);
    m_ui.SpinBoxInt("Tint", tint, 0, 100);
    m_ui.SpinBoxInt("Saturation", sat, 0, 100);
    m_ui.Break();
    m_ui.Action("Reset");
    m_ui.CheckBox("Enabled", true);
    m_ui.EndList();
}

void MenuSystem::BlendMode(int id, int* selectedIdx)
{
    m_ui.MenuTitle("FX -> BlendMode");

}


void MenuSystem::OutputPlanes(int id, int* selectedIdx)
{
    m_ui.MenuTitle("OUT");
    m_ui.BeginList(selectedIdx);
    if(m_ui.Action("Plane 1")){
        setMenu(MT_PlaneSettings);
    } else if (m_ui.Action("Plane 2")){
        setMenu(MT_PlaneSettings);
    } else if (m_ui.Action("Plane 3")) {
        setMenu(MT_PlaneSettings);
    } else if (m_ui.Action("Plane 4")) {
        setMenu(MT_PlaneSettings);
    }
    m_ui.EndList();
}

void MenuSystem::PlaneSettings(int id, int* selectedIdx)
{
    m_ui.MenuTitle("OUT -> Plane #");
    m_ui.BeginList(selectedIdx);
    if (m_ui.Action("Mask")){
        setMenu(MT_Mask);
    } else if (m_ui.Action("Mapping")) {
        setMenu(MT_Mapping);
    }
    m_ui.Break();
    if(m_ui.Action("HDMI Channel")){
        setMenu(MT_HdmiSelection);
    }
    m_ui.EndList();

}

void MenuSystem::HdmiSelection(int id, int* selectedIdx)
{
    m_ui.MenuTitle("Out -> Plane # -> HDMI Channel");
    m_ui.BeginList(selectedIdx);
    m_ui.RadioButton("HDMI Output 1", true);
    m_ui.RadioButton("HDMI Output 2", false);
    m_ui.EndList();
}

void MenuSystem::Mask(int id, int* selectedIdx)
{
    m_ui.MenuTitle("Out -> Plane # -> Mask");
    m_ui.BeginList(selectedIdx);
    m_ui.Text("some way to load image or create a mask...");
    m_ui.EndList();
}

void MenuSystem::Mapping(int id, int* selectedIdx)
{
    m_ui.MenuTitle("Out -> Plane # -> Mapping");
    m_ui.BeginList(selectedIdx);
    m_ui.Text("Amazing Mapping Mode...");
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
    std::string useFader = "Use Fader (" + std::to_string(settings.analog0).substr(0, 4) + ")";
    if(m_ui.CheckBox(useFader, settings.useFader)) { settings.useFader = !settings.useFader; }
    std::string useRotaryAsFader = "Use Rotary (" + std::to_string(settings.rotary) + ")" ;
    if(m_ui.CheckBox(useRotaryAsFader, settings.useRotaryAsFader)) { settings.useRotaryAsFader = !settings.useRotaryAsFader; }
    m_ui.SpinBoxInt("Fade Time", settings.fadeTime, 0, 10);
    m_ui.SpinBoxInt("Volume", settings.volume, 0, 10);
    //if (m_registry.settings().isProVersion) { m_ui.SpinBoxInt("Rot. Sensit.", settings.rotarySensitivity, 1, 20) };
    if (m_ui.CheckBox("Use UVC", settings.useUvcCaptureDevice)) { settings.useUvcCaptureDevice = !settings.useUvcCaptureDevice; };
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
        else if (ShaderInputConfig* shaderInputConfig = dynamic_cast<ShaderInputConfig*>(currentConfig)) {
            if (shaderInputConfig->isActive) m_buttonTexts[i].second = COLOR::GREEN;
            else m_buttonTexts[i].second = COLOR::DARK_BLUE;
        }
    }
    m_ui.ShowButtonMatrix(m_buttonTexts);
}