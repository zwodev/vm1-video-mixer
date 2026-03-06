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

MenuSystem::MenuSystem(UI& ui, Registry& registry, EventBus& eventBus) : 
    m_ui(ui), 
    m_registry(registry), 
    m_eventBus(eventBus)
{   
    subscribeToEvents();
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

void MenuSystem::SubMenu(const std::string& label, std::function<void()> func)
{
    bool selected = m_ui.Text(label);
    if  (selected && m_ui.isNavigationEventTriggered(NavigationEvent::Type::NavigationRight)) {
        m_currentMenuPath.push_back(MenuState(m_focusedIdx, m_currentMenuFunc));
        m_focusedIdx = 0;
        m_currentMenuFunc = func;
    }
}

void MenuSystem::setMenu(MenuType menuType)
{
    bool changed = true;
    switch (menuType) {
        case MT_StartupScreen:
            m_currentMenuFunc = [this](){ StartupScreen(); };
            break;
        case MT_InfoMenu:
            m_currentMenuFunc = [this](){ InfoMenu(); };
            break;
        case MT_SourceMenu:
            m_currentMenuFunc = [this](){ SourceMenu(); };
            break;
        case MT_ControlMenu:
            m_currentMenuFunc = [this](){ ControlMenu(); };
            break;
        case MT_FxMenu:
            m_currentMenuFunc = [this](){ FxMenu(); };
            break;
        case MT_OutputMenu:
            m_currentMenuFunc = [this](){ OutputMenu(); };
            break;
        case MT_NetworkMenu:
            m_currentMenuFunc = [this](){ NetworkMenu(); };
            break;
        case MT_GlobalSettingsMenu:
            m_currentMenuFunc = [this](){ GlobalSettingsMenu(); };
            break;
        case MT_DeviceSettingsMenu:
            m_currentMenuFunc = [this](){ DeviceSettingsMenu(); };
            break;
        case MT_ButtonMatrixMenu:
            m_currentMenuFunc = [this](){ ButtonMatrixMenu(); };
            break;
        default:
            changed = false;

    }

    if (changed)
    {
        m_currentMenuType = menuType;
        m_focusedIdx = 0;
        m_currentMenuPath.clear();
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

void MenuSystem::handleStringInputDialog()
{
    if(m_showStringInputDialog){
        m_ui.ShowStringInputDialog("Create Folder", m_stringInputDialogCursorIdx, m_stringInputDialogString);
    }
}

void MenuSystem::handleMediaAndEditButtons()
{
    // check the media-slot-ids
    for (int mediaSlotId : m_ui.getTriggeredMediaSlotIds())
    {
        m_id = mediaSlotId;
        return;
    }

    // check the edit-buttons
    for (int editButtonId : m_ui.getTriggeredEditButtons())
    {
        std::cout << "editButtonId: " << editButtonId << std::endl;

        switch (editButtonId)
        {
        case 0:
            setMenu(MT_InfoMenu);
            break;
        case 1:
            setMenu(MT_SourceMenu);
            break;
        case 2:
            setMenu(MT_ControlMenu);
            break;
        case 3:
            setMenu(MT_FxMenu);
            break;
        case 4:
            setMenu(MT_OutputMenu);
            break;
        case 5:
            setMenu(MT_DeviceSettingsMenu);
            break;
        case 6:
            setMenu(MT_GlobalSettingsMenu);
            break;
        case 7:
            if(!m_registry.settings().kiosk.enabled) {
                setMenu(MT_NetworkMenu);
            } else {
                showPopupMessage("Not implemented");
            }
            break;
        case 8:     // 8-15: Edit Keys with Fn-Button pressed
            setMenu(MT_ButtonMatrixMenu);
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
    if(m_ui.isNavigationEventTriggered(NavigationEvent::Type::NavigationDown))
    {
        m_ui.FocusNextElement();
    }
    else if(m_ui.isNavigationEventTriggered(NavigationEvent::Type::NavigationUp))
    {
        m_ui.FocusPreviousElement();
    }
}

void MenuSystem::handleBankSwitching()
{
    int bank = 0;
    if (m_ui.isBankChangeEventTriggered(bank)) {
        m_registry.inputMappings().bank = bank;
        m_ui.StartOverlay([this](){m_ui.ShowBankInfo(m_registry.inputMappings().bank);});
    }
}

void MenuSystem::goUpHierachy() {
    if (!m_currentMenuPath.empty()) {
        MenuState menuState = m_currentMenuPath.back();
        m_focusedIdx = menuState.fIdx;
        m_currentMenuFunc = menuState.func;
        m_currentMenuPath.pop_back();
    }
}

void MenuSystem::handleMenuHierachyNavigation()
{
    if (m_ui.isNavigationEventTriggered(NavigationEvent::Type::NavigationLeft)) 
    {
        goUpHierachy();
    }
}

void MenuSystem::render()
{
    m_ui.NewFrame();

    // Render bank information
    if (m_currentMenuType == MT_InfoMenu  || 
        m_currentMenuType == MT_SourceMenu || 
        m_currentMenuType == MT_ControlMenu ||
        m_currentMenuType == MT_ButtonMatrixMenu) 
    {
        int id16 = (m_id % MEDIA_BUTTON_COUNT) + 1;
        char bank = m_id / MEDIA_BUTTON_COUNT + 65; // "+65" to get ASCII code
        std::string mediaSlotString = std::string(1, bank) + std::to_string(id16);

        m_ui.MenuInfo(mediaSlotString);
    }


    // Render dynamic content (if present)
    if (m_currentMenuFunc) {
        m_currentMenuFunc();
    }

    // Render Overlay
    m_ui.ShowOverlay();

    handlePopupMessage();
    handleStringInputDialog();
    handleMediaAndEditButtons();
    handleUpAndDownKeys();    
    handleBankSwitching();
    handleMenuHierachyNavigation();


    m_ui.EndFrame();
}

void MenuSystem::ClearSlot() {
    m_registry.inputMappings().removeConfig(m_id);
    goUpHierachy();
}

// ##### STARTUP SCREEN #####
void MenuSystem::StartupScreen()
{
    if(m_registry.settings().displayType == DisplayType::SSD1351_OLED) { 
        m_ui.startUpLogo();
    } else if(m_registry.settings().displayType == DisplayType::ILI9341_IPS_LCD) {
        m_ui.Image(ImageBuffer("media/splash-screen.png"));
    }
}

// ##### INFO MENU #####
void MenuSystem::InfoMenu()
{
    m_ui.MenuTitle("Info");
    m_ui.BeginList(&m_focusedIdx);
    InputConfig *inputConfig = m_registry.inputMappings().getInputConfig(m_id);
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

// ##### SOURCE MENU #####
void MenuSystem::SourceMenu() 
{
    m_ui.MenuTitle("SRC");
    m_ui.BeginList(&m_focusedIdx);
    SubMenu("Media Files", [this](){ FileSelection(); });
    SubMenu("HDMI Inputs", [this](){ LiveInputSelection(); });
    SubMenu("Shaders", [this](){ ShaderSelection(); });
    SubMenu("Clear Slot", [this](){ ClearSlot(); }); // TODO: Use Action.
    m_ui.EndList();
}

void MenuSystem::FileSelection()
{
    m_ui.MenuTitle("SRC/Files");
    auto config = std::make_unique<VideoInputConfig>();
    config->looping = m_registry.settings().defaultLooping;

    VideoInputConfig* currentConfig = m_registry.inputMappings().getVideoInputConfig(m_id);
    if (currentConfig) {
        *config = *currentConfig;
    } 

    std::vector<std::string>& files = m_registry.mediaPool().getVideoFiles();
    bool changed = false;
    m_ui.BeginList(&m_focusedIdx);
    if(m_ui.Action("New Folder") && !m_showStringInputDialog) {
        printf("Create New Folder\n");
        m_stringInputDialogString = "neu";
        m_showStringInputDialog = true;
    }
    else if(m_ui.Action("USB-Drive")) {
        printf("Enter USB-Drive\n");
    }
    m_ui.Break();
    for (int i = 0; i < files.size(); ++i) {
        std::string fileName = files[i];
        if (m_ui.RadioButton(fileName.c_str(), (config->fileName == fileName))) {
            config->fileName = fileName;
            changed = true;
        }
    }
    m_ui.EndList(); 

    if (changed)
        m_registry.inputMappings().addInputConfig(m_id, std::move(config));
}

void MenuSystem::LiveInputSelection() 
{
    m_ui.MenuTitle("SRC/Live");
    auto config = std::make_unique<HdmiInputConfig>();
    HdmiInputConfig* currentConfig = m_registry.inputMappings().getHdmiInputConfig(m_id);
    if (currentConfig) { *config = *currentConfig; }

    bool changed = false;
    m_ui.BeginList(&m_focusedIdx);
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
        m_registry.inputMappings().addInputConfig(m_id, std::move(config));
    }
}

void MenuSystem::ShaderSelection()
{
    m_ui.MenuTitle("SRC/Shaders");
    auto config = std::make_unique<ShaderInputConfig>();

    ShaderInputConfig* currentConfig = m_registry.inputMappings().getShaderInputConfig(m_id);
    if (currentConfig) {
        *config = *currentConfig;
    } 

    std::vector<std::string>& files = m_registry.mediaPool().getShaderFiles();
    bool changed = false;
    m_ui.BeginList(&m_focusedIdx);
    for (int i = 0; i < files.size(); ++i) {
        std::string fileName = files[i];
        if (m_ui.RadioButton(fileName.c_str(), (config->fileName == fileName))) {
            config->fileName = fileName;
            changed = true;
        }
    }
    m_ui.EndList(); 

    if (changed)
        m_registry.inputMappings().addInputConfig(m_id, std::move(config));
}

// ##### CONTROL MENU #####
void MenuSystem::ControlMenu() 
{
    m_ui.MenuTitle("CTRL");
    InputConfig* currentConfig = m_registry.inputMappings().getInputConfig(m_id);
    if (!currentConfig) {
        m_ui.BeginList(&m_focusedIdx);
        m_ui.Text("No input selected");
        m_ui.EndList();
        return;
    }
    
    if (VideoInputConfig* videoInputConfig = dynamic_cast<VideoInputConfig*>(currentConfig)) {
        m_ui.BeginList(&m_focusedIdx);
        m_ui.Text("Type: Mediafile");
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
        m_ui.BeginList(&m_focusedIdx);
        m_ui.Text("Type: HDMI");
        std::string inputName = "Source: HDMI" + std::to_string(hdmiInputConfig->hdmiPort+1);
        m_ui.Text(inputName);
    }
    else if (dynamic_cast<ShaderInputConfig*>(currentConfig)) {
        m_ui.BeginList(&m_focusedIdx);
        m_ui.Text("Type: Shader");
        m_ui.Text("Parameter 1");
        m_ui.Text("Parameter 2");
        m_ui.Text("Parameter 3");
    }
    m_ui.Break();
    int p = 1;
    m_ui.SpinBoxInt("Out Plane", p, 1, 4);
    m_ui.EndList();
}

// ##### FX MENU #####
// TODO
// {"ChromaKey", {}, [this](int id, int* fIdx){ChromaKey(id, fIdx);}},
// {"CustomFx", {}, [this](int id, int* fIdx){CustomFx(id, fIdx);}},
// {"ColorCorrection", {}, [this](int id, int* fIdx){ColorCorrection(id, fIdx);}},
// {"BlendMode", {}, [this](int id, int* fIdx){BlendMode(id, fIdx);}}
void MenuSystem::FxMenu()
{
    m_ui.MenuTitle("FX");
    m_ui.BeginList(&m_focusedIdx);
    auto& effects = m_registry.planeSettings().effects;

    for (int i = 0; i < effects.size(); ++i) {
        SubMenu(effects[i].name, [this](){ EffectControl(); });
    }
    m_ui.EndList();
}

void MenuSystem::EffectControl()
{
    
    auto& effects = m_registry.planeSettings().effects;
    int effectIndex = 0; // TODO: Save this in variable.
    auto& effect = m_registry.planeSettings().effects[effectIndex];
    m_ui.MenuTitle("FX/" + effect.name);

    m_ui.BeginList(&m_focusedIdx);
    for (int i = 0; i < effect.params.size(); ++i) {
        auto& param = effect.params[i];
        if (std::holds_alternative<IntParameter>(param)) {
            auto& intParam = std::get<IntParameter>(param);  
            m_ui.SpinBoxInt(intParam.name, intParam.value, intParam.min, intParam.max, intParam.step);
        } else if (std::holds_alternative<FloatParameter>(param)) {
            auto& floatParam = std::get<FloatParameter>(param); 
            m_ui.SpinBoxFloat(floatParam.name, floatParam.value, floatParam.min, floatParam.max, floatParam.step);
        } else if (std::holds_alternative<ColorParameter>(param)) {
            // add ColorParam
        }
        
    }
    m_ui.EndList(); 
}

// ##### OUT MENU #####
void MenuSystem::OutputMenu()
{
    m_ui.MenuTitle("OUT");
    m_ui.BeginList(&m_focusedIdx);
    SubMenu("Mrs. Mask", [this](){ Mask(); });
    SubMenu("Mr. Mapping", [this](){ Mapping(); });
    SubMenu("HDMI Output", [this](){ HdmiSelection(); });
    m_ui.EndList();
}

void MenuSystem::Mask()
{
    m_ui.MenuTitle("OUT/Mask");
    m_ui.BeginList(&m_focusedIdx);
    m_ui.Text("some way to load image or create a mask...");
    m_ui.EndList();
}

void MenuSystem::Mapping()
{
    m_ui.MenuTitle("OUT/Mapping");
    m_ui.BeginList(&m_focusedIdx);

    m_ui.SpinBoxVec2("TopLeft", m_registry.planeSettings().coords[3]); 
    m_ui.SpinBoxVec2("TopRight", m_registry.planeSettings().coords[2]); 
    m_ui.SpinBoxVec2("BottomRight", m_registry.planeSettings().coords[1]); 
    m_ui.SpinBoxVec2("BottomLeft", m_registry.planeSettings().coords[0]); 
    m_ui.Break();
    m_ui.SpinBoxInt("Rotation", m_registry.planeSettings().rotation,0, 360, 1);
    m_ui.SpinBoxFloat("Scale", m_registry.planeSettings().scale, 0.0f, 10.0f, 0.1f);
    m_ui.SpinBoxVec2("ScaleXY", m_registry.planeSettings().scaleXY);
    m_ui.SpinBoxVec2("Translation", m_registry.planeSettings().translation);
    m_ui.Break();
    if(m_ui.Action("Reset")) {
        m_registry.planeSettings().resetMapping();
    }
    m_ui.EndList();
}



// ##### NETWORK MENU #####
void MenuSystem::NetworkMenu() 
{
    m_ui.MenuTitle("Network");
    Settings& settings = m_registry.settings();
    std::string eth0;
    std::string wlan0;
    NetworkTools::getIPAddress("eth0", eth0);
    NetworkTools::getIPAddress("wlan0", wlan0);

    m_ui.BeginList(&m_focusedIdx);
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

// GLOBAL SETTINGS MENU
void MenuSystem::GlobalSettingsMenu() 
{
    m_ui.MenuTitle("Settings");
    Settings& settings = m_registry.settings();

    m_ui.BeginList(&m_focusedIdx);
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
    m_ui.SpinBoxInt("ScreenRotation", (int&)settings.hdmiRotation0, 0, 3);
    m_ui.EndList();
}

// ##### DEVICE SETTINGS MENU #####
void MenuSystem::DeviceSettingsMenu() 
{  
    m_ui.MenuTitle("Hdmi Settings");
    Settings& settings = m_registry.settings();

    m_ui.BeginList(&m_focusedIdx);
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

// ##### KEYS/MATRIX MENU #####
void MenuSystem::ButtonMatrixMenu() 
{
    m_ui.MenuTitle("Button");
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
            else m_buttonTexts[i].second = COLOR::DARK_GREEN;
        }
    }
    m_ui.ShowButtonMatrix(m_buttonTexts);
}

void MenuSystem::HdmiSelection()
{
    m_ui.BeginList(&m_focusedIdx);
    m_ui.RadioButton("HDMI Output 1", true);
    m_ui.RadioButton("HDMI Output 2", false);
    m_ui.EndList();
}

// ##### PROTOTYPING #####
// void MenuSystem::Effects()
// {
//     m_ui.BeginList(&m_focusedIdx);
//     if(m_ui.Action("[ ] Chroma Key")){
//         setMenu(MT_ChromaKey);
//     } else if (m_ui.Action("[ ] Custom Effect")){
//         setMenu(MT_CustomFx);
//     } else if (m_ui.Action("[x] Color Correction")) {
//         setMenu(MT_ColorCorrection);
//     } else if (m_ui.Action("[x] Blend Mode")) {
//         setMenu(MT_BlendMode);
//     }
//     m_ui.EndList();

// }

// void MenuSystem::CustomFx()
// {
//     m_ui.BeginList(&m_focusedIdx);
//     m_ui.Text("If no fx is selected, file-list is visible.");
//     m_ui.Text("If fx is selected, parameters are visible,");
//     m_ui.Break();
//     m_ui.RadioButton("file-1.frag", false);
//     m_ui.RadioButton("file-2.frag", false);
//     m_ui.RadioButton("file-3.frag", false);
//     m_ui.RadioButton("file-4.frag", false);
//     m_ui.Break();
//     m_ui.Action("Reset");
//     m_ui.Action("Remove (only visible when fx-shader active)");
//     m_ui.CheckBox("Enabled", true);
//     m_ui.EndList();
// }

// void MenuSystem::ChromaKey()
// {
//     m_ui.BeginList(&m_focusedIdx);
//     m_ui.Text("Color");
//     int tolerance = 50, smoothness = 50, spill = 50;
//     m_ui.SpinBoxInt("Tolerance", tolerance, 0, 100);
//     m_ui.SpinBoxInt("Smoothness", smoothness, 0, 100);
//     m_ui.SpinBoxInt("Spill", spill, 0, 100);
//     m_ui.Text("Pre-Mask");
//     m_ui.Break();
//     m_ui.Action("Reset");
//     m_ui.CheckBox("Enabled", true);
//     m_ui.EndList();
// }

// void MenuSystem::ColorCorrection()
// {
//     m_ui.BeginList(&m_focusedIdx);
//     int black = 0, white = 100, gamma = 50, temperature = 50, tint = 50, sat = 50;
//     m_ui.SpinBoxInt("Black-Point", black, 0, 100);
//     m_ui.SpinBoxInt("White-Point", white, 0, 100);
//     m_ui.SpinBoxInt("Gamma", gamma, 0, 100);
//     m_ui.SpinBoxInt("Temperature", temperature, 0, 100);
//     m_ui.SpinBoxInt("Tint", tint, 0, 100);
//     m_ui.SpinBoxInt("Saturation", sat, 0, 100);
//     m_ui.Break();
//     m_ui.Action("Reset");
//     m_ui.CheckBox("Enabled", true);
//     m_ui.EndList();
// }

// void MenuSystem::BlendMode()
// {

// }


