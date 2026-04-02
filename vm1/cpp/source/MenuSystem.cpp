/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
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

#include <chrono>   // only needed for screenshot timestamp
#include <iomanip>  // only needed for screenshot timestamp
#include <sstream>  // only needed for screenshot timestamp

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

bool MenuSystem::SubMenu(const std::string& label, std::function<void()> func, SubMenuType subMenuType)
{
    bool selected = m_ui.Text(label);
    if  (selected && m_ui.isNavigationEventTriggered(NavigationEvent::Type::NavigationRight)) {
        m_currentMenuPath.push_back(MenuState(m_focusedIdx, m_currentMenuFunc));
        m_focusedIdx = 0;
        m_currentMenuFunc = func;
        m_currentSubMenuType = subMenuType;
        return true;
    }
    return false;
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
        m_currentSubMenuType = SMT_None;
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
        InputConfig* inputConfig = m_registry.inputMappings().getInputConfig(m_id);
        if (inputConfig)
        {
            m_planeIdx = inputConfig->planeId;        
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

void MenuSystem::handlePlaneSwitching()
{
    int diff = 0;
    if(m_ui.isNavigationEventTriggered(NavigationEvent::Type::NavigationAuxDown))
    {
        diff = 1;        
    }
    else if(m_ui.isNavigationEventTriggered(NavigationEvent::Type::NavigationAuxUp))
    {
        diff = -1;
    }
    else 
    {
        return;
    }
    
    int* id = nullptr;
    if (m_currentMenuType == MT_SourceMenu || 
        m_currentMenuType == MT_ControlMenu ) 
    {
        // change output plane for the currently selected media slot
        InputConfig* inputConfig = m_registry.inputMappings().getInputConfig(m_id);
        if (inputConfig) {
            id = &inputConfig->planeId;
        } 
    }
    else if (m_currentMenuType == MT_FxMenu ||
             m_currentMenuType == MT_OutputMenu)
    {
        // select output plane for FX or OUT menu
        id = &m_planeIdx;
    }

    if (id != nullptr)
    {
        *id += diff;
        int minValue = 0;
        int maxValue = m_registry.planes().size() - 1;
        if (*id > maxValue) *id = maxValue;
        if (*id < minValue) *id = minValue;

        if (m_currentMenuType == MT_SourceMenu || 
            m_currentMenuType == MT_ControlMenu ) 
        {
            m_planeIdx = *id; // switch output plane as well
        }
    }


}

void MenuSystem::goUpHierachy() {
    if (!m_currentMenuPath.empty()) {
        MenuState menuState = m_currentMenuPath.back();
        m_focusedIdx = menuState.fIdx;
        m_currentMenuFunc = menuState.func;
        m_currentMenuPath.pop_back();
        m_currentSubMenuType = SMT_None; // ToDo: add a proper stack for submenutypes
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

    if (m_currentMenuType == MT_InfoMenu  || 
        m_currentMenuType == MT_SourceMenu || 
        m_currentMenuType == MT_ControlMenu ||
        m_currentMenuType == MT_ButtonMatrixMenu) 
    {        
        int id16 = (m_id % MEDIA_BUTTON_COUNT) + 1;
        char bank = m_id / MEDIA_BUTTON_COUNT + 65; // "+65" to get ASCII code
        int planeId = 0;
        std::string mediaSlotString = std::string(1, bank) + std::to_string(id16);
        InputConfig* inputConfig = m_registry.inputMappings().getInputConfig(m_id);
        if (inputConfig) {
            planeId =  inputConfig->planeId;
            // mediaSlotString += ">>" + std::to_string(planeId + 1);
        }
        m_ui.PlanePreview(m_registry.planes(), planeId, m_selectedVertexId, UI::PlanePreviewStyle::PLANE_PREVIEW_SMALL);
        m_ui.ShowMediaSlotInfo(mediaSlotString);
    } 
    else if (m_currentMenuType == MT_FxMenu ||
             m_currentMenuType == MT_OutputMenu)
    {
        UI::PlanePreviewStyle previewStyle = UI::PlanePreviewStyle::PLANE_PREVIEW_LARGE;
        if(m_currentSubMenuType == SMT_Mapping) {
            previewStyle = UI::PlanePreviewStyle::PLANE_PREVIEW_VERTICES;
        }
        m_ui.Spacer(40.0);
        m_ui.PlanePreview(m_registry.planes(), m_planeIdx, m_selectedVertexId, previewStyle);
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
    handlePlaneSwitching();
    
    // Take a screenshot
    if (m_ui.isNavigationEventTriggered(NavigationEvent::Type::Screenshot)) 
    {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d-%H-%M-%S") << ".png";
        std::string filename = oss.str();
        
        printf("Screenshot %s\n", filename.c_str());
        m_ui.savePNG("screenshots/" + filename);
    }

    m_ui.EndFrame();
}

void MenuSystem::ClearSlot() {
    m_registry.inputMappings().removeConfig(m_id);
    goUpHierachy();
}

// ##### STARTUP SCREEN #####
void MenuSystem::StartupScreen()
{
    m_ui.Image(m_registry.mediaPool().getLogo());
}

// ##### INFO MENU #####
void MenuSystem::InfoMenu()
{
    m_ui.ShowMenuTitle("Info");
    m_ui.Spacer(m_ui.getMenuTitleHeight());
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
        std::string previewFilename = m_registry.mediaPool().getVideoFilePath(videoInputConfig->fileName) + ".preview";
        m_ui.MediaPreview(previewFilename);
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
    m_ui.ShowMenuTitle("SRC");
    m_ui.Spacer(m_ui.getMenuTitleHeight());
    m_ui.pushTranslate(50, 40);

    m_ui.TextStyle(FONT::TEXTSTYLE::ROOT_MENU_ITEM);
    m_ui.BeginList(&m_focusedIdx);
    m_ui.TextColor(COLOR::YELLOW);
    SubMenu("MEDIA", [this](){ FileSelection(); });
    m_ui.TextColor(COLOR::MAGENTA);
    SubMenu("HDMI", [this](){ LiveInputSelection(); });
    m_ui.TextColor(COLOR::CYAN);
    SubMenu("SHADER", [this](){ ShaderSelection(); });
    m_ui.Spacer();
    m_ui.TextColor(COLOR::WHITE);
    SubMenu("CLEAR SLOT", [this](){ ClearSlot(); }); // TODO: Use Action.
    m_ui.EndList();
    m_ui.popTranslate();
    m_ui.TextStyle(FONT::TEXTSTYLE::STANDARD);
}

void MenuSystem::FileSelection()
{
    m_ui.ShowMenuTitle("MEDIA");
    m_ui.Spacer(m_ui.getMenuTitleHeight());

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
    else if(m_ui.Action("Generate Previews")) {
        m_eventBus.publish(CreateMediaPreviewEvent());
    }
    m_ui.Spacer();
    m_ui.TextStyle(FONT::TEXTSTYLE::LIST_ITEM);
    for (size_t i = 0; i < files.size(); ++i) {
        std::string fileName = files[i];
        if (m_ui.RadioButton(fileName.c_str(), (config->fileName == fileName))) {
            config->fileName = fileName;
            changed = true;
        }
    }
    m_ui.TextStyle(FONT::TEXTSTYLE::STANDARD);
    m_ui.EndList(); 

    if(m_focusedIdx >=3)
    {
        if(int(files.size()) > (m_focusedIdx-3) && (m_focusedIdx-3) >= 0){
            std::string previewFilename = m_registry.mediaPool().getVideoFilePath(files[m_focusedIdx-3]) + ".preview";    
            m_ui.MediaPreview(previewFilename);
        }
    }

    if (changed)
        m_registry.inputMappings().addInputConfig(m_id, std::move(config));
}

void MenuSystem::LiveInputSelection() 
{
    m_ui.ShowMenuTitle("HDMI");
    m_ui.Spacer(m_ui.getMenuTitleHeight());

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
    m_ui.ShowMenuTitle("SHADER");
    m_ui.Spacer(m_ui.getMenuTitleHeight());

    auto config = std::make_unique<ShaderInputConfig>();

    ShaderInputConfig* currentConfig = m_registry.inputMappings().getShaderInputConfig(m_id);
    if (currentConfig) {
        *config = *currentConfig;
    } 

    std::vector<std::string>& files = m_registry.mediaPool().getGenerativeShaderFiles();
    bool changed = false;
    m_ui.BeginList(&m_focusedIdx);
    for (size_t i = 0; i < files.size(); ++i) {
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
    m_ui.ShowMenuTitle("CTL");
    m_ui.Spacer(m_ui.getMenuTitleHeight());
    m_ui.pushTranslate(0, 20);

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
    else if (ShaderInputConfig* shaderInputConfig = dynamic_cast<ShaderInputConfig*>(currentConfig)) {
        m_ui.BeginList(&m_focusedIdx);
        m_ui.Text("Type: Shader");
        m_ui.Text("Name: " + shaderInputConfig->fileName);
        for (auto& kv : shaderInputConfig->shaderConfig.params) {
            auto& param = kv.second;
            if (std::holds_alternative<IntParameter>(param)) {
                auto& intParam = std::get<IntParameter>(param);  
                m_ui.SpinBoxInt(intParam.name, intParam.value, intParam.min, intParam.max, intParam.step);
            } else if (std::holds_alternative<FloatParameter>(param)) {
                auto& floatParam = std::get<FloatParameter>(param); 
                m_ui.SpinBoxFloat(floatParam.name, floatParam.value, floatParam.min, floatParam.max, floatParam.step);
            }
        }
    }
    m_ui.Spacer();
    
    // NOTE: Testing -> Preview only the plane that this slot is associated with
    // if(m_ui.SpinBoxInt("Out Plane", currentConfig->planeId , 0, m_registry.planes().size()-1)){
    //     m_planeIdx = currentConfig->planeId;
    // }
    // m_ui.previewPlanes(m_registry.planes(), currentConfig->planeId);

    m_ui.EndList();
    m_ui.popTranslate();
}

// ##### FX MENU #####
// TODO
// {"ChromaKey", {}, [this](int id, int* fIdx){ChromaKey(id, fIdx);}},
// {"CustomFx", {}, [this](int id, int* fIdx){CustomFx(id, fIdx);}},
// {"ColorCorrection", {}, [this](int id, int* fIdx){ColorCorrection(id, fIdx);}},
// {"BlendMode", {}, [this](int id, int* fIdx){BlendMode(id, fIdx);}}
void MenuSystem::FxMenu()
{
    m_ui.ShowMenuTitle("FX");
    m_ui.Spacer(m_ui.getMenuTitleHeight());
    m_ui.pushTranslate(0, 100);

    m_ui.BeginList(&m_focusedIdx);
    PlaneSettings& plane = m_registry.planes()[m_planeIdx];
    const ShaderConfig& shaderConfig = plane.shaderConfig;
    for (const auto& kv : shaderConfig.groups) {
        const std::string& groupName = kv.first;
        bool isSelectedEffect = SubMenu(groupName, [this](){ EffectControl(); });
        if (isSelectedEffect) {
            m_effectName = groupName;
        }
    }
    SubMenu("Select Custom Shader", [this](){ CustomEffectShaderSelection(); });
    if (m_ui.Action("Clear Custom Shader")) {
        plane.extShaderFilename = "";
        m_eventBus.publish(EffectShaderEvent(m_planeIdx));
    }
    m_ui.EndList();
    m_ui.popTranslate();
}

void MenuSystem::CustomEffectShaderSelection()
{
    m_ui.ShowMenuTitle("CUSTOM EFFECTS");
    m_ui.Spacer(m_ui.getMenuTitleHeight());
    m_ui.pushTranslate(0, 100);

    PlaneSettings& plane = m_registry.planes()[m_planeIdx];

    std::vector<std::string>& files = m_registry.mediaPool().getEffectShaderFiles();
    m_ui.BeginList(&m_focusedIdx);
    for (size_t i = 0; i < files.size(); ++i) {
        std::string fileName = files[i];
        if (m_ui.RadioButton(fileName.c_str(), (plane.extShaderFilename == fileName))) {
            plane.extShaderFilename = fileName;
            m_eventBus.publish(EffectShaderEvent(m_planeIdx));
        }
    }
    m_ui.EndList();
    m_ui.popTranslate();
}

void MenuSystem::EffectControl()
{
    auto& shaderConfig = m_registry.planes()[m_planeIdx].shaderConfig;
    if (!shaderConfig.groups.contains(m_effectName)) return;

    auto& group = shaderConfig.groups[m_effectName];
    m_ui.ShowMenuTitle("FX/" + m_effectName);
    m_ui.Spacer(m_ui.getMenuTitleHeight());

    m_ui.pushTranslate(0, 100);

    m_ui.BeginList(&m_focusedIdx);
    for (const auto& paramName : group) {
        if (!shaderConfig.params.contains(paramName)) continue;
        auto& param = shaderConfig.params[paramName];
        if (std::holds_alternative<IntParameter>(param)) {
            auto& intParam = std::get<IntParameter>(param);  
            m_ui.SpinBoxInt(intParam.name, intParam.value, intParam.min, intParam.max, intParam.step);
        } else if (std::holds_alternative<FloatParameter>(param)) {
            auto& floatParam = std::get<FloatParameter>(param); 
            m_ui.SpinBoxFloat(floatParam.name, floatParam.value, floatParam.min, floatParam.max, floatParam.step);
        } 
    }
    m_ui.EndList(); 
    m_ui.popTranslate();
}

// ##### OUT MENU #####
void MenuSystem::OutputMenu()
{
    m_ui.ShowMenuTitle("OUT");
    m_ui.Spacer(m_ui.getMenuTitleHeight());

    m_ui.pushTranslate(0, 100);
    m_ui.BeginList(&m_focusedIdx);
    SubMenu("Mrs. Mask", [this](){ Mask(); });
    SubMenu("Mr. Mapping", [this](){ Mapping(); }, SMT_Mapping);
    m_ui.SpinBoxInt("Blend Mode", (int&)m_registry.planes()[m_planeIdx].blendMode, 0, 2);
    m_ui.SpinBoxFloat("Opacity", m_registry.planes()[m_planeIdx].opacity, 0.0f, 1.0f);
    if(m_ui.CheckBox("Use Fader For Opacity", m_registry.planes()[m_planeIdx].useFaderForOpacity))
    {
        m_registry.planes()[m_planeIdx].useFaderForOpacity = !m_registry.planes()[m_planeIdx].useFaderForOpacity;
    }
    m_ui.SpinBoxInt("HDMI Output", m_registry.planes()[m_planeIdx].hdmiId, 0, 1);
    m_ui.EndList();
    m_ui.popTranslate();
}

void MenuSystem::Mask()
{
    m_ui.ShowMenuTitle("OUT/Mask");
    m_ui.Spacer(m_ui.getMenuTitleHeight());
    m_ui.pushTranslate(0, 100);

    m_ui.BeginList(&m_focusedIdx);
    m_ui.Text("some way to load image or create a mask...");
    m_ui.EndList();
    m_ui.popTranslate();
}

void MenuSystem::Mapping()
{
    m_ui.ShowMenuTitle("OUT/Mapping");
    m_ui.Spacer(m_ui.getMenuTitleHeight());
    m_ui.pushTranslate(0, 100);
    
    m_ui.BeginList(&m_focusedIdx);
    // printf("%d\n", m_ui.m_y);

    m_ui.HideElements();
    // rendering of the upcoming elements is done in another element (PlanePreview()).
    // PlanePreview() is also using m_selectedVertexId to visualize the selected vertex.
    m_selectedVertexId = m_focusedIdx;
    m_ui.SpinBoxVec2("TopLeft", m_registry.planes()[m_planeIdx].coords[3]); 
    m_ui.SpinBoxVec2("TopRight", m_registry.planes()[m_planeIdx].coords[2]); 
    m_ui.SpinBoxVec2("BottomRight", m_registry.planes()[m_planeIdx].coords[1]); 
    m_ui.SpinBoxVec2("BottomLeft", m_registry.planes()[m_planeIdx].coords[0]); 
    m_ui.ShowElements();

    // m_ui.SpinBoxInt("Rotation", m_registry.planes()[m_planeIdx].rotation,0, 360, 1);
    m_ui.SpinBoxFloat("Scale", m_registry.planes()[m_planeIdx].scale, 0.0f, 10.0f, 0.1f);
    // m_ui.SpinBoxVec2("ScaleXY", m_registry.planes()[m_planeIdx].scaleXY);
    m_ui.SpinBoxVec2("Translation", m_registry.planes()[m_planeIdx].translation);
    m_ui.Spacer();
    if(m_ui.Action("Reset")) {
        m_registry.planes()[m_planeIdx].resetMapping();
    }
    m_ui.EndList();
    m_ui.popTranslate();
}



// ##### NETWORK MENU #####
void MenuSystem::NetworkMenu() 
{
    m_ui.ShowMenuTitle("Network");
    m_ui.Spacer(m_ui.getMenuTitleHeight());

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
    m_ui.ShowMenuTitle("Settings");
    m_ui.Spacer(m_ui.getMenuTitleHeight());

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
    m_ui.ShowMenuTitle("Hdmi Settings");
    m_ui.Spacer(m_ui.getMenuTitleHeight());

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
    m_ui.ShowMenuTitle("Button");
    m_ui.Spacer(m_ui.getMenuTitleHeight());

    int bank = m_registry.inputMappings().bank;
    for (size_t i = 0; i < m_buttonTexts.size(); ++i) {
        int mediaSlot = bank * 16 + int(i);
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
//     m_ui.Spacer();
//     m_ui.RadioButton("file-1.frag", false);
//     m_ui.RadioButton("file-2.frag", false);
//     m_ui.RadioButton("file-3.frag", false);
//     m_ui.RadioButton("file-4.frag", false);
//     m_ui.Spacer();
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
//     m_ui.Spacer();
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
//     m_ui.Spacer();
//     m_ui.Action("Reset");
//     m_ui.CheckBox("Enabled", true);
//     m_ui.EndList();
// }

// void MenuSystem::BlendMode()
// {

// }


