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
#include <filesystem>

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

bool MenuSystem::SubMenu(const std::string& label, std::function<void()> func)
{
    bool focused = m_ui.Text(label);
    if  (focused && m_ui.isNavigationEventTriggered(NavigationEvent::Type::NavigationRight)) {
        m_currentMenuPath.push_back(MenuState(0, func));
        return true;
    }
    return false;
}

bool MenuSystem::SubDir(const std::string& label, std::function<void()> func)
{
    bool focused = m_ui.Text(label);
    if  (focused && m_ui.isNavigationEventTriggered(NavigationEvent::Type::NavigationRight)) {
        m_currentMenuPath.push_back(MenuState(0, func, label));
        return true;
    }
    return false;
}

void MenuSystem::setMenu(MenuType menuType)
{
    bool changed = true;
    std::function<void()> menuFunc;
    switch (menuType) {
        case MT_StartupScreen:
            menuFunc = [this](){ StartupScreen(); };
            break;
        case MT_InfoMenu:
            menuFunc = [this](){ InfoMenu(); };
            break;
        case MT_SourceMenu:
            menuFunc = [this](){ SourceMenu(); };
            break;
        case MT_ControlMenu:
            menuFunc = [this](){ ControlMenu(); };
            break;
        case MT_FxMenu:
            menuFunc = [this](){ FxMenu(); };
            break;
        case MT_OutputMenu:
            menuFunc = [this](){ OutputMenu(); };
            break;
        case MT_NetworkMenu:
            menuFunc = [this](){ NetworkMenu(); };
            break;
        case MT_GlobalSettingsMenu:
            menuFunc = [this](){ GlobalSettingsMenu(); };
            break;
        case MT_DeviceSettingsMenu:
            menuFunc = [this](){ DeviceSettingsMenu(); };
            break;
        case MT_ButtonMatrixMenu:
            menuFunc = [this](){ ButtonMatrixMenu(); };
            break;
        default:
            changed = false;
    }

    if (changed)
    {
        m_currentMenuType = menuType;
        m_currentMenuPath.clear();
        m_currentMenuPath.push_back(MenuState(0, menuFunc));
    }
}

void MenuSystem::showPopupMessage(const std::string& message) 
{
    m_popUp.message = message;
    m_popUp.show = true;
}

void MenuSystem::handlePopupMessage()
{
    if (m_popUp.show && !m_popUp.message.empty()) {
        m_ui.StartOverlay([this]() { m_ui.ShowPopupMessage(m_popUp.message); });
        m_popUp.show = false;
    }
}

void MenuSystem::handleInputDialog()
{
    bool isClosing = false;
    if(m_inputDialog.show) {
        if (m_ui.isNavigationEventTriggered(NavigationEvent::Type::NavigationLeft)) 
        {
            isClosing = true;
        }

        BDF::TextStyle inputStyle = BDF::TEXTSTYLE::ROOT_MENU_ITEM;
        inputStyle.align = TextAlign::LEFT;
        inputStyle.color = COLOR::YELLOW;
        m_ui.TextStyle(inputStyle);
        m_ui.BeginList(&m_currentMenuPath.back().fIdx);
        m_ui.ShowInputDialog("Create Folder", m_inputDialog.cursorIdx, m_inputDialog.text);
        m_ui.Spacer();
        m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
        if(m_ui.Action("OK"))
        {
            std::string videoPath = m_registry.mediaPool().getVideoFilesPath();
            std::filesystem::create_directory(videoPath + m_inputDialog.text);
            isClosing = true;
        }
        else if(m_ui.Action("Cancel"))
        {
            isClosing = true;
        }
        m_ui.EndList();
    }

    if(isClosing)
    {
        m_currentMenuPath.back().fIdx = 0; // todo: restore previous cursor position
        m_inputDialog.show = false;
    }
}

void MenuSystem::handleMediaAndEditButtons()
{
    // check the media-slot-ids
    for (int mediaSlotId : m_ui.getTriggeredMediaSlotIds())
    {
        m_activeMediaSlot.slotId = mediaSlotId;
        // Shortcut (set selected Plane in FX/OUT for convenience)
        InputConfig* inputConfig = m_registry.inputMappings().getInputConfig(m_activeMediaSlot.slotId);
        if (inputConfig)
        {
            m_activeOutputPlane.planeId = inputConfig->planeId;        
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
        m_ui.StartOverlay([this](){m_ui.BankInfoWidget(m_registry.inputMappings().bank);});
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
        printf("Media Slot: %d\n", m_activeMediaSlot.slotId); 
        // change output plane for the currently selected media slot
        InputConfig* inputConfig = m_registry.inputMappings().getInputConfig(m_activeMediaSlot.slotId, true);
        if (inputConfig) {
            id = &inputConfig->planeId;
            printf("Active Slot: %d\n", *id);   
        } 
    }
    else if (m_currentMenuType == MT_FxMenu ||
             m_currentMenuType == MT_OutputMenu)
    {
        // select output plane for FX or OUT menu
        id = &m_activeOutputPlane.planeId;
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
            m_activeOutputPlane.planeId = *id; // switch output plane as well
        }
    }
}

void MenuSystem::goUpHierachy() {
    if (m_currentMenuPath.size() > 1) {
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
    if (m_inputDialog.show)
    {
        m_ui.NewFrame(false);
        handleInputDialog();
        handleUpAndDownKeys();
        m_ui.EndFrame();
        return;
    }

    m_ui.NewFrame();

    // get infos for top menu titles
    int id16 = (m_activeMediaSlot.slotId % MEDIA_BUTTON_COUNT) + 1;
    char bank = m_activeMediaSlot.slotId / MEDIA_BUTTON_COUNT + 65; // "+65" to get ASCII code
    m_activeMediaSlot.slotName = std::string(1, bank) + std::to_string(id16);
    InputConfig* inputConfig = m_registry.inputMappings().getInputConfig(m_activeMediaSlot.slotId, true);
    if (inputConfig) m_activeMediaSlot.planeId =  inputConfig->planeId;
    else m_activeMediaSlot.planeId = -1;   

    // Render dynamic content (if present)
    if (m_currentMenuPath.back().func) {
        m_currentMenuPath.back().func();
    }

    // Render Overlay
    m_ui.ShowOverlay();

    
    handlePopupMessage();
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
    m_registry.inputMappings().removeConfig(m_activeMediaSlot.slotId);
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
    m_ui.MenuTitleWidget("Info", TextAlign::CENTER);
    m_ui.MenuTitleWidget(m_activeMediaSlot.slotName, TextAlign::LEFT);
    m_ui.PlanePreviewWidget(m_registry.planes(), m_activeMediaSlot.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_SMALL);
    m_ui.NewLine();
    
    
    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    InputConfig *inputConfig = m_registry.inputMappings().getInputConfig(m_activeMediaSlot.slotId);
    if (VideoInputConfig *videoInputConfig = dynamic_cast<VideoInputConfig *>(inputConfig))
    {
        m_ui.Label("Source:    Media File");
        m_ui.Label("Filename:  " + videoInputConfig->fileName);
        m_ui.Label("Duration:  ");
        m_ui.Label("In-Point:  ");
        m_ui.Label("Out-Point: ");
        m_ui.Label("Current Position:");
        std::string previewFilename = videoInputConfig->fileName + ".preview";
        MediaPreview(previewFilename);
    }
    else if (HdmiInputConfig*hdmiInputConfig = dynamic_cast<HdmiInputConfig *>(inputConfig))
    {
        m_ui.Label("Source:     HDMI");
        m_ui.Label("Input Port: " + std::to_string(hdmiInputConfig->hdmiPort));
        m_ui.Label("Resolution: ");
    }
    else if (ShaderInputConfig*shaderInputConfig = dynamic_cast<ShaderInputConfig *>(inputConfig))
    {
        m_ui.Label("Source:     Shader");
        m_ui.Label("Filename:   " + shaderInputConfig->fileName);
        m_ui.Label("Parameters: ");
    }
    m_ui.EndList(); 
}

// ##### SOURCE MENU #####
void MenuSystem::SourceMenu() 
{
    m_ui.MenuTitleWidget("SRC", TextAlign::CENTER);
    m_ui.MenuTitleWidget(m_activeMediaSlot.slotName, TextAlign::LEFT);
    m_ui.PlanePreviewWidget(m_registry.planes(), m_activeMediaSlot.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_SMALL);
    m_ui.NewLine();

    m_ui.PushTranslate(0, 40);

    m_ui.TextStyle(BDF::TEXTSTYLE::ROOT_MENU_ITEM);
    m_ui.BeginList(&m_currentMenuPath.back().fIdx);

    m_ui.TextColor(COLOR::YELLOW);
    SubMenu("MEDIA", [this](){ FileSelection(); });
    int hSpace = 8;
    m_ui.Spacer(hSpace);

    m_ui.TextColor(COLOR::MAGENTA);
    SubMenu("LIVE", [this](){ LiveInputSelection(); });
    m_ui.Spacer(hSpace);

    m_ui.TextColor(COLOR::CYAN);
    SubMenu("SHADER", [this](){ ShaderSelection(); });
    m_ui.Spacer(hSpace*2);

    m_ui.TextColor(COLOR::WHITE);
    BDF::TextStyle textStyle = BDF::TEXTSTYLE::MENU_ITEM;
    textStyle.align = TextAlign::CENTER;
    m_ui.TextStyle(textStyle);
    if (m_ui.Action("CLEAR SLOT"))
    {
        ClearSlot();
    }
    m_ui.EndList();
    m_ui.PopTranslate();
}

std::string MenuSystem::currentDirectoryPath()
{
    std::string path;
    for (const MenuState& menuState : m_currentMenuPath) {
        if (!menuState.name.empty()) {
            path += menuState.name + "/";
        }
    }
    if (!m_currentMenuPath.back().name.empty()) {
        path += m_currentMenuPath.back().name + "/";
    }
    return path;
}

void MenuSystem::MediaPreview(const std::string& filename)
{
    std::shared_ptr<PreviewNode> previewNode = m_registry.mediaPool().getPreview(filename);
    if(m_preview.imageFileName != filename)
    {
        if (previewNode->image.isValid) {
            m_preview.imageFileName = filename;
            m_preview.frameIndex = 0;
        }
    }

    if (previewNode->image.isValid) {
        m_ui.AnimationFrameWidget(previewNode->image, m_preview.frameIndex);
    }
}

void MenuSystem::FileSelection()
{
    m_ui.MenuTitleWidget("MEDIA", TextAlign::CENTER);
    m_ui.MenuTitleWidget(m_activeMediaSlot.slotName, TextAlign::LEFT);
    m_ui.PlanePreviewWidget(m_registry.planes(), m_activeMediaSlot.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_SMALL);
    m_ui.NewLine();

    m_ui.PushTranslate(4, 20);
    auto config = std::make_unique<VideoInputConfig>();
    config->looping = m_registry.settings().defaultLooping;

    VideoInputConfig* currentConfig = m_registry.inputMappings().getVideoInputConfig(m_activeMediaSlot.slotId, true);
    if (currentConfig) {
        *config = *currentConfig;
    }

    std::string videoPath = currentDirectoryPath();
    std::vector<DirectoryEntry> entries = m_registry.mediaPool().getVideoDirectoryEntries(videoPath);
    //printf("VideoPath: %s Entries: %ld\n", videoPath.c_str(), entries.size());
    bool changed = false;
    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
    if(m_ui.Action("New Folder") && !m_inputDialog.show) {
        m_inputDialog.cursorIdx = 0;
        m_inputDialog.text = "noname";
        m_inputDialog.show = true;
    }
    if(m_ui.Action("USB-Drive")) {
        printf("Enter USB-Drive\n");
    }

    m_ui.Spacer();
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM_MONOSPACED);
    int fileListStartIdx = m_ui.CurrentListSize();
    for (size_t i = 0; i < entries.size(); ++i) {
        const DirectoryEntry& entry = entries[i];
        if (entry.isDir) {
            SubDir(entry.name, [this]() { FileSelection(); });
        }
        else {
            if (m_ui.RadioButton(entry.name.c_str(), (config->fileName == entry.absolutePath))) {
                config->fileName = entry.absolutePath;
                changed = true;
            }
        }
    }
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
    m_ui.EndList(); 

    if(m_currentMenuPath.back().fIdx >= fileListStartIdx)
    {
        int fileIndex = m_currentMenuPath.back().fIdx-fileListStartIdx;
        if(int(entries.size()) > fileIndex && fileIndex >= 0) {
            const DirectoryEntry& entry = entries[fileIndex];
            if (!entry.isDir) {
                std::string videoFilePath = entry.absolutePath;
                std::string previewFilename = videoFilePath + ".preview";    
                MediaPreview(previewFilename);
            }
        }
    }

    if (changed)
        m_registry.inputMappings().stageInputConfig(m_activeMediaSlot.slotId, std::move(config));
    
    m_ui.PopTranslate();
}

void MenuSystem::LiveInputSelection() 
{
    m_ui.MenuTitleWidget("LIVE", TextAlign::CENTER);
    m_ui.MenuTitleWidget(m_activeMediaSlot.slotName, TextAlign::LEFT);
    m_ui.PlanePreviewWidget(m_registry.planes(), m_activeMediaSlot.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_SMALL);
    m_ui.NewLine();
    
    m_ui.PushTranslate(4, 20);
    auto config = std::make_unique<HdmiInputConfig>();
    HdmiInputConfig* currentConfig = m_registry.inputMappings().getHdmiInputConfig(m_activeMediaSlot.slotId, true);
    if (currentConfig) { *config = *currentConfig; }

    bool changed = false;
    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
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
        m_registry.inputMappings().stageInputConfig(m_activeMediaSlot.slotId, std::move(config));
    }
    m_ui.PopTranslate();
}

void MenuSystem::ShaderSelection()
{
    m_ui.MenuTitleWidget("SHADER", TextAlign::CENTER);
    m_ui.MenuTitleWidget(m_activeMediaSlot.slotName, TextAlign::LEFT);
    m_ui.PlanePreviewWidget(m_registry.planes(), m_activeMediaSlot.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_SMALL);
    m_ui.NewLine();

    m_ui.PushTranslate(4, 20);
    auto config = std::make_unique<ShaderInputConfig>();

    ShaderInputConfig* currentConfig = m_registry.inputMappings().getShaderInputConfig(m_activeMediaSlot.slotId, true);
    if (currentConfig) {
        *config = *currentConfig;
    } 

    std::string shaderPath = currentDirectoryPath();
    std::vector<DirectoryEntry> entries = m_registry.mediaPool().getGenerativeShaderFiles(shaderPath);

    bool changed = false;
    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM_MONOSPACED);
    for (size_t i = 0; i < entries.size(); ++i) {
        const DirectoryEntry& entry = entries[i];
        if (entry.isDir) {
            SubDir(entry.name, [this]() { ShaderSelection(); });
        }
        else if (m_ui.RadioButton(entry.name.c_str(), (config->fileName == entry.absolutePath))) {
            config->fileName = entry.absolutePath;
            changed = true;
        }
    }
    m_ui.EndList(); 

    if (changed)
        m_registry.inputMappings().stageInputConfig(m_activeMediaSlot.slotId, std::move(config));

    m_ui.PopTranslate();
}

// ##### CONTROL MENU #####
void MenuSystem::ControlMenu() 
{
    m_ui.MenuTitleWidget("CTL", TextAlign::CENTER);
    m_ui.MenuTitleWidget(m_activeMediaSlot.slotName, TextAlign::LEFT);
    m_ui.NewLine();

    m_ui.PushTranslate(4, 20);
    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);

    InputConfig* currentConfig = m_registry.inputMappings().getInputConfig(m_activeMediaSlot.slotId);
    if (!currentConfig) {
        m_ui.Label("No input selected");
        m_ui.EndList();
        return;
    }
    
    if (VideoInputConfig* videoInputConfig = dynamic_cast<VideoInputConfig*>(currentConfig)) {
        m_ui.Label("Type: Mediafile");
        if (m_ui.CheckBox("loop", videoInputConfig->looping)) { 
            videoInputConfig->looping = !videoInputConfig->looping; 
        }
        if (m_ui.CheckBox("backwards", videoInputConfig->backwards)) {
            videoInputConfig->backwards = !videoInputConfig->backwards;
        }
        m_ui.Label("start-time");
        m_ui.Label("end-time");
        m_ui.Label("speed");
    }
    else if (HdmiInputConfig* hdmiInputConfig = dynamic_cast<HdmiInputConfig*>(currentConfig)) {
        m_ui.Label("Type: HDMI");
        std::string inputName = "Source: HDMI" + std::to_string(hdmiInputConfig->hdmiPort+1);
        m_ui.Label(inputName);
    }
    else if (ShaderInputConfig* shaderInputConfig = dynamic_cast<ShaderInputConfig*>(currentConfig)) {
        m_ui.Label("Type: Shader");
        m_ui.Label("Name: " + shaderInputConfig->fileName);
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
    
    m_ui.EndList();
    // m_ui.PopTranslate();
}

// ##### FX MENU #####
// TODO
// {"ChromaKey", {}, [this](int id, int* fIdx){ChromaKey(id, fIdx);}},
// {"CustomFx", {}, [this](int id, int* fIdx){CustomFx(id, fIdx);}},
// {"ColorCorrection", {}, [this](int id, int* fIdx){ColorCorrection(id, fIdx);}},
// {"BlendMode", {}, [this](int id, int* fIdx){BlendMode(id, fIdx);}}
void MenuSystem::FxMenu()
{
    m_ui.MenuTitleWidget("FX", TextAlign::CENTER);
    m_ui.NewLine();

    m_ui.PlanePreviewWidget(m_registry.planes(), m_activeOutputPlane.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_LARGE);
    m_ui.NewLine();

    m_ui.PushTranslate(0, 20);

    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
    PlaneSettings& plane = m_registry.planes()[m_activeOutputPlane.planeId];
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
        m_eventBus.publish(EffectShaderEvent(m_activeOutputPlane.planeId));
    }
    m_ui.EndList();
    m_ui.PopTranslate();
}

void MenuSystem::CustomEffectShaderSelection()
{
    m_ui.MenuTitleWidget("CUSTOM EFFECTS", TextAlign::CENTER);
    m_ui.NewLine();

    m_ui.PlanePreviewWidget(m_registry.planes(), m_activeOutputPlane.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_LARGE);
    m_ui.NewLine();

    m_ui.PushTranslate(0, 20);

    PlaneSettings& plane = m_registry.planes()[m_activeOutputPlane.planeId];

    std::string shaderPath = currentDirectoryPath();
    std::vector<DirectoryEntry> entries = m_registry.mediaPool().getEffectShaderFiles(shaderPath);

    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM_MONOSPACED);
    for (size_t i = 0; i < entries.size(); ++i) {
        const DirectoryEntry& entry = entries[i];
        if (entry.isDir) {
            SubDir(entry.name, [this]() { CustomEffectShaderSelection(); });
        }
        else if (m_ui.RadioButton(entry.name.c_str(), (plane.extShaderFilename == entry.absolutePath))) {
            plane.extShaderFilename = entry.absolutePath;
            m_eventBus.publish(EffectShaderEvent(m_activeOutputPlane.planeId));
        }
    }
    m_ui.EndList();
    m_ui.PopTranslate();
}

void MenuSystem::EffectControl()
{
    auto& shaderConfig = m_registry.planes()[m_activeOutputPlane.planeId].shaderConfig;
    if (!shaderConfig.groups.contains(m_effectName)) return;

    auto& group = shaderConfig.groups[m_effectName];
    m_ui.MenuTitleWidget("FX/" + m_effectName, TextAlign::CENTER);
    m_ui.NewLine();
    m_ui.PlanePreviewWidget(m_registry.planes(), m_activeOutputPlane.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_LARGE);
    m_ui.NewLine();
    m_ui.PushTranslate(0, 20);

    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
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
    m_ui.PopTranslate();
}

// ##### OUT MENU #####
void MenuSystem::OutputMenu()
{
    m_ui.MenuTitleWidget("OUT", TextAlign::CENTER);
    m_ui.NewLine();
    m_ui.PlanePreviewWidget(m_registry.planes(), m_activeOutputPlane.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_LARGE);
    m_ui.NewLine();
    m_ui.PushTranslate(0, 20);

    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    SubMenu("Mrs. Mask", [this](){ Mask(); });
    SubMenu("Mr. Mapping", [this](){ Mapping(); });
    m_ui.SpinBoxInt("Blend Mode", (int&)m_registry.planes()[m_activeOutputPlane.planeId].blendMode, 0, 2);
    m_ui.SpinBoxFloat("Opacity", m_registry.planes()[m_activeOutputPlane.planeId].opacity, 0.0f, 1.0f);
    if(m_ui.CheckBox("Use Fader For Opacity", m_registry.planes()[m_activeOutputPlane.planeId].useFaderForOpacity))
    {
        m_registry.planes()[m_activeOutputPlane.planeId].useFaderForOpacity = !m_registry.planes()[m_activeOutputPlane.planeId].useFaderForOpacity;
    }
    m_ui.SpinBoxInt("HDMI Output", m_registry.planes()[m_activeOutputPlane.planeId].hdmiId, 0, 1);
    m_ui.EndList();
    m_ui.PopTranslate();
}

void MenuSystem::Mask()
{
    m_ui.MenuTitleWidget("MASK", TextAlign::CENTER);
    m_ui.NewLine();
    m_ui.PlanePreviewWidget(m_registry.planes(), m_activeOutputPlane.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_LARGE);
    m_ui.NewLine();
    m_ui.PushTranslate(0, 20);

    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
    m_ui.Label("some way to load image or create a mask...");
    m_ui.EndList();
    m_ui.PopTranslate();
}

void MenuSystem::Mapping()
{
    m_ui.MenuTitleWidget("MAPPING", TextAlign::CENTER);
    m_ui.NewLine();

    m_activeOutputPlane.selectedVertexId =  m_currentMenuPath.back().fIdx - m_ui.CurrentListSize();
    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.PlanePreviewWidget(m_registry.planes(), m_activeOutputPlane.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_VERTICES);
    m_ui.NewLine();
    m_ui.PushTranslate(0, 20);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
    m_ui.SpinBoxFloat("Scale", m_registry.planes()[m_activeOutputPlane.planeId].scale, 0.0f, 10.0f, 0.1f);
    m_ui.SpinBoxVec2("Translation", m_registry.planes()[m_activeOutputPlane.planeId].translation);
    m_ui.Spacer();
    if(m_ui.Action("Reset")) {
        m_registry.planes()[m_activeOutputPlane.planeId].resetMapping();
    }
    m_ui.EndList();
    m_ui.PopTranslate();
}



// ##### NETWORK MENU #####
void MenuSystem::NetworkMenu() 
{
    m_ui.MenuTitleWidget("Network", TextAlign::CENTER);
    m_ui.NewLine();

    std::string eth0;
    std::string wlan0;
    NetworkTools::getIPAddress("eth0", eth0);
    NetworkTools::getIPAddress("wlan0", wlan0);

    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
    if (!eth0.empty()) m_ui.Label("e: " + eth0);
    if (!wlan0.empty()) m_ui.Label("w: " + wlan0);
    m_ui.Label("SSID: VM-1");
    m_ui.Label("Pass: vmone12345");
    m_ui.EndList();
    
    // TODO: Add QR code for WiFi
    // const ImageBuffer& imageBuffer = m_registry.mediaPool().getQrCodeImageBuffer();
    // if (imageBuffer.isValid) {
    //     m_ui.Image(imageBuffer);
    // }
}

// GLOBAL SETTINGS MENU
void MenuSystem::GlobalSettingsMenu() 
{
    m_ui.MenuTitleWidget("Settings", TextAlign::CENTER);
    m_ui.NewLine();

    Settings& settings = m_registry.settings();

    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
    m_ui.Label("Version: " + VERSION);
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
    m_ui.MenuTitleWidget("Hdmi Settings", TextAlign::CENTER);
    m_ui.NewLine();

    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
    if (m_registry.settings().isHdmiOutputReady && m_registry.settings().isHdmiInputReady) {
        if (m_ui.Action("Scan for new")) {
            m_registry.settings().isHdmiOutputReady = false;
            m_registry.settings().isHdmiInputReady = false;
            m_eventBus.publish(SystemEvent(SystemEvent::Type::Restart));  
        }
    }
    else {
        m_ui.Label("Scanning...");
    }
    
    auto& hdmiOutputs = m_registry.settings().hdmiOutputs;
    for (size_t i = 0; i < hdmiOutputs.size(); ++i) {
        std::string displayConfig = !(hdmiOutputs[i].empty()) ? hdmiOutputs[i] : "Not connected";
        m_ui.Label(std::string("O") + std::to_string(i+1) + ": " + displayConfig);
    }

    auto& hdmiInputs = m_registry.settings().hdmiInputs;
    for (size_t i = 0; i < hdmiInputs.size(); ++i) {
        std::string inputConfig = !(hdmiInputs[i].empty()) ? hdmiInputs[i] : "Not connected";
        m_ui.Label(std::string("I") + std::to_string(i+1) + ": " + inputConfig);
    }

    m_ui.EndList();
}

// ##### KEYS/MATRIX MENU #####
void MenuSystem::ButtonMatrixMenu() 
{
    m_ui.MenuTitleWidget("Button");
    m_ui.NewLine();

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
    m_ui.ButtonMatrixWidget(m_buttonTexts);
}
