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
#include <iostream>
#include <ranges>
#include <filesystem>
#include <algorithm>


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
    // if  (focused && m_ui.isNavigationEventTriggered(NavigationEvent::Type::NavigationRight)) {
    if  (focused && m_ui.isNavigationEventTriggered(NavigationEvent::Type::NavigationEnter)) {
        appendStateToMenuPath(MenuState(0, func));
        return true;
    }
    return false;
}

bool MenuSystem::SubDir(const std::string& label, std::function<void()> func)
{
    bool focused = m_ui.Text(label);
    // if  (focused && m_ui.isNavigationEventTriggered(NavigationEvent::Type::NavigationRight)) {
    if  (focused && m_ui.isNavigationEventTriggered(NavigationEvent::Type::NavigationEnter)) {
        appendStateToMenuPath(MenuState(0, func, label));
        return true;
    }
    return false;
}

std::vector<std::string> MenuSystem::splitPath(const std::string& path) {
    std::vector<std::string> result;

    for (auto&& part : path | std::views::split('/')) {
        std::string segment(part.begin(), part.end());

        if (segment.empty() || segment == "." || segment == "..")
            continue;

        result.push_back(segment);
    }

    return result;
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
        case MT_SettingsMenu:
            menuFunc = [this](){ SettingsMenu(); };
            break;
        // case MT_NetworkMenu:
        //     menuFunc = [this](){ NetworkMenu(); };
        //     break;
        // case MT_DeviceSettingsMenu:
        //     menuFunc = [this](){ DeviceSettingsMenu(); };
        //     break;
        // case MT_ButtonMatrixMenu:
        //     menuFunc = [this](){ ButtonMatrixMenu(); };
        //     break;
        default:
            changed = false;
    }

    if (changed)
    {
        m_currentMenuType = menuType;
        m_currentMenuPath.clear();
        m_currentMenuPath.push_back(MenuState(0, menuFunc));
        m_ui.setClearFrame(true);
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

void MenuSystem::TextInputDialog()
{
    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.ShowTextInputDialog(m_textInputDialog.title, m_textInputDialog.cursorIdx, m_textInputDialog.text);
    m_ui.Spacer();
    
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
    if(m_ui.Action("OK"))
    {
        m_textInputDialog.func();
        goUpHierachy();
    }
    else if(m_ui.Action("Cancel"))
    {
        goUpHierachy();
    }
    m_ui.EndList();
}

void MenuSystem::FolderSelectionDialog()
{
    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.ShowDialog(m_folderSelectionDialog.title, m_folderSelectionDialog.subtitle);
    m_ui.Spacer();
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM_MONOSPACED);

    for(const auto& folder : m_folderSelectionDialog.subFolders)
    {
        if(m_ui.RadioButton(std::string(folder).replace(0,9,""), m_folderSelectionDialog.foldername == folder)) // remove "../videos" prefix
        {
            m_folderSelectionDialog.foldername = folder;
        }
        // m_ui.Text(std::string(folder).replace(0, 9, "")); 
    }
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
    if(m_ui.Action("OK"))
    {
        m_folderSelectionDialog.func();
        goUpHierachy();
    }
    else if(m_ui.Action("Cancel"))
    {
        goUpHierachy();
    }
    m_ui.EndList();
}

void MenuSystem::ConfirmActionDialog()
{
    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.ShowDialog(m_confirmActionDialog.title, m_confirmActionDialog.subtitle);
    m_ui.Spacer();

    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM_MONOSPACED);
    m_ui.Label(m_confirmActionDialog.text);
    m_ui.Spacer();

    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
    if(m_ui.Action("OK"))
    {
        m_confirmActionDialog.func();
        goUpHierachy();
    }
    else if(m_ui.Action("Cancel"))
    {
        goUpHierachy();
    }
    m_ui.EndList();
}

void MenuSystem::FileManagerDialog()
{
    m_ui.BeginList(&m_currentMenuPath.back().fIdx);

    m_ui.ShowDialog(m_fileManagerDialog.entry.name);
    if(m_ui.Action("Select"))
    {
        if(m_fileManagerDialog.fileType == FileManagerDialogData::FileType::Video) {
            std::unique_ptr config = std::make_unique<VideoInputConfig>();
            config->fileName = m_fileManagerDialog.entry.absolutePath;
            config->planeId = m_activeOutputPlane.planeId;
            m_registry.inputMappings().stageInputConfig(m_fileManagerDialog.slotId, std::move(config));
        } 
        else if (m_fileManagerDialog.fileType == FileManagerDialogData::FileType::Shader) {
            std::unique_ptr config = std::make_unique<ShaderInputConfig>();
            config->fileName = m_fileManagerDialog.entry.absolutePath;
            config->planeId = m_activeOutputPlane.planeId;
            m_registry.inputMappings().stageInputConfig(m_fileManagerDialog.slotId, std::move(config));
        }
        goUpHierachy();
    }
    else if(SubMenu("Rename", [this](){ TextInputDialog(); })) 
    {
        m_textInputDialog.cursorIdx = 0;
        m_textInputDialog.title = "Rename";
        m_textInputDialog.text = m_fileManagerDialog.entry.name;
        m_textInputDialog.func = [this](){
            printf("%s\n", m_fileManagerDialog.entry.absolutePath.c_str());
            std::string oldAbsFilename =  m_fileManagerDialog.entry.absolutePath;
            std::string newAbsFilename =  m_fileManagerDialog.entry.directory + "/" + m_textInputDialog.text;
            printf("Renaming: %s to %s\n", oldAbsFilename.c_str(), newAbsFilename.c_str());
            // todo: make sure no preview-creating-process starts here?
            if (std::filesystem::exists(oldAbsFilename)) {
                std::filesystem::rename(oldAbsFilename, newAbsFilename);
                m_fileManagerDialog.entry.name = m_textInputDialog.text;
                m_fileManagerDialog.entry.absolutePath = newAbsFilename;

                // re-link the new filename to the corresponding media-slot
                // BUG: right now, the current mediaSlot (m_fileManagerDialog.slotId) gets the renamed file, no matter if it was previously selected for this slot or not.
                // TODO: check if the filename is connected to *any* media-slot and change it there - this has nothing to do with the currently selected mediaSlot!!
                // Q: would it be good if the FileManager shows information if/where a file is used?
                if(m_fileManagerDialog.fileType == FileManagerDialogData::FileType::Video) { 
                    /*
                     Q: Do I really need to make a new VideoInputConfig-Pointer and move that back to the inputMappings?
                        Can't I just grab the current VideoInputConfig and change that fileName?
                    */                    
                    // VideoInputConfig* config = m_registry.inputMappings().getVideoInputConfig(m_fileManagerDialog.slotId);
                    // config->fileName = newAbsFilename;

                    std::unique_ptr config = std::make_unique<VideoInputConfig>();
                    VideoInputConfig* currentConfig = m_registry.inputMappings().getVideoInputConfig(m_fileManagerDialog.slotId);
                    if(currentConfig)
                    {
                        *config = *currentConfig;
                        config->fileName = newAbsFilename;
                        m_registry.inputMappings().stageInputConfig(m_fileManagerDialog.slotId, std::move(config));
                    }
                } 
                else if (m_fileManagerDialog.fileType == FileManagerDialogData::FileType::Shader) {
                    std::unique_ptr config = std::make_unique<ShaderInputConfig>();
                    ShaderInputConfig* currentConfig = m_registry.inputMappings().getShaderInputConfig(m_fileManagerDialog.slotId);
                    if(currentConfig)
                    {
                        *config = *currentConfig;
                        config->fileName = newAbsFilename;
                        m_registry.inputMappings().stageInputConfig(m_fileManagerDialog.slotId, std::move(config));
                    }
                }

            } else {
                printf("error while renaming - file not found\n");
            }
            if (std::filesystem::exists(oldAbsFilename + ".preview")) {
                std::filesystem::rename(oldAbsFilename + ".preview", newAbsFilename + ".preview");
            }
            
        };
    }
    else if(SubMenu("Move", [this](){FolderSelectionDialog();})) 
    {
        m_folderSelectionDialog.title = "Move";
        m_folderSelectionDialog.subtitle = m_fileManagerDialog.entry.name;
        m_folderSelectionDialog.foldername = m_registry.mediaPool().getVideoFilePath();
        m_folderSelectionDialog.subFolders.clear();
        m_folderSelectionDialog.subFolders.push_back(std::filesystem::path(m_folderSelectionDialog.foldername));
        for (const auto& entry : std::filesystem::recursive_directory_iterator(m_folderSelectionDialog.foldername))
        {
            if (entry.is_directory())
            m_folderSelectionDialog.subFolders.push_back(entry.path());
        }
        m_folderSelectionDialog.func = [this](){
            std::string sourceFile = m_registry.mediaPool().getVideoFilePath(currentDirectoryPath() + m_fileManagerDialog.entry.name);
            std::string destinationFile = m_folderSelectionDialog.foldername + "/" + m_fileManagerDialog.entry.name;

            printf("Move: %s to %s\n", sourceFile.c_str(), destinationFile.c_str());
            std::filesystem::copy_file(sourceFile, destinationFile);
            std::filesystem::copy_file(sourceFile + ".preview", destinationFile + ".preview");
            std::filesystem::remove(sourceFile);
            std::filesystem::remove(sourceFile + ".preview");
            goUpHierachy();
        };
    }
    // else if(SubMenu("Copy", [this](){FolderSelectionDialog();})) 
    // {
    //     m_folderSelectionDialog.title = "Copy";
    //     m_folderSelectionDialog.subtitle = m_fileManagerDialog.entry.name;
    //     m_folderSelectionDialog.foldername = m_registry.mediaPool().getVideoFilePath();
    //     m_folderSelectionDialog.subFolders.clear();
    //     m_folderSelectionDialog.subFolders.push_back(std::filesystem::path(m_folderSelectionDialog.foldername));
    //     for (const auto& entry : std::filesystem::recursive_directory_iterator(m_folderSelectionDialog.foldername))
    //     {
    //         if (entry.is_directory())
    //             m_folderSelectionDialog.subFolders.push_back(entry.path());
    //     }
    //     m_folderSelectionDialog.func = [this](){
    //         std::string sourceFile = m_registry.mediaPool().getVideoFilePath(currentDirectoryPath() + m_fileManagerDialog.entry.name);
    //         std::string destinationFile = m_folderSelectionDialog.foldername + "/" + m_fileManagerDialog.entry.name;

    //         printf("Copy: %s to %s\n", sourceFile.c_str(), destinationFile.c_str());
    //         std::filesystem::copy_file(sourceFile, destinationFile);
    //         std::filesystem::copy_file(sourceFile + ".preview", destinationFile + ".preview");
    //         goUpHierachy();
    //     };
    // }
    else if(SubMenu("Delete", [this](){ConfirmActionDialog();}))  
    {
        m_confirmActionDialog.title = "Delete";
        m_confirmActionDialog.subtitle = m_fileManagerDialog.entry.name;
        m_confirmActionDialog.text = "Are you sure?";
        m_confirmActionDialog.func = [this](){
            std::string filePath = m_registry.mediaPool().getVideoFilePath(currentDirectoryPath() + m_fileManagerDialog.entry.name);
            printf("Deleting %s\n", filePath.c_str());
            std::filesystem::remove(filePath);
            std::filesystem::remove(filePath + ".preview");
            goUpHierachy();
        };
    }
    // else if(SubMenu("Convert to clip (n/a)", [this](){ConfirmActionDialog();})) 
    // {
    //     m_confirmActionDialog.title = "Convert to Clip";
    //     m_confirmActionDialog.subtitle = m_fileManagerDialog.entry.name;
    //     m_confirmActionDialog.text = "This might take a while.\nAre you sure?";
    //     m_confirmActionDialog.func = [this](){
    //         printf("TODO: Converting... %s\n", m_fileManagerDialog.entry.name.c_str());
    //     };
    // }
    else if(m_ui.Action("Cancel"))
    {
        goUpHierachy();
    }
    m_ui.EndList();
}

void MenuSystem::handleMediaAndEditButtons()
{
    // check the media-slot-ids
    for (int mediaSlotId : m_ui.getTriggeredMediaSlotIds())
    {
        m_registry.inputMappings().setFocusedMediaSlot(mediaSlotId);
        InputConfig* inputConfig = m_registry.inputMappings().getInputConfig(mediaSlotId);
        if (!inputConfig) return;

        if (m_currentMenuType == MT_SourceMenu) {
            setMenu(MT_SourceMenu);
            SelectActiveSourceFolder();
        }
        
        m_activeOutputPlane.planeId = inputConfig->planeId;        
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
            if (m_currentMenuType != MT_SourceMenu) {
                setMenu(MT_SourceMenu);
                m_showActivesSource = false;
            }
            else if (m_currentMenuPath.size() == 1) {
                m_showActivesSource = true;
                SelectActiveSourceFolder();
            }
            else {
                setMenu(MT_SourceMenu);
                m_showActivesSource = !m_showActivesSource;
                if(m_showActivesSource) SelectActiveSourceFolder();
            }
            
            printf("m_focusActiveSource: %d\n", m_focusActiveSource);
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
            setMenu(MT_SettingsMenu);
        break;
        case 6:
            // setMenu(MT_DeviceSettingsMenu);
            break;
        case 7:
            // if(!m_registry.settings().kiosk.enabled) {
            //     setMenu(MT_NetworkMenu);
            // } else {
            //     showPopupMessage("Not implemented");
            // }
            break;
        case 8:     // 8-15: Edit Keys with Fn-Button pressed
            // setMenu(MT_ButtonMatrixMenu);
            break;
        // case 15:
            // setMenu(MT_StartupScreen);
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
    if (m_ui.isBankChangeEventTriggered(bank)) 
    {
        m_registry.inputMappings().focusedBank = bank;
        m_ui.StartOverlay([this, bank](){m_ui.BankInfoWidget(bank);});
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
    
    int* planeId = nullptr;
    
    if (m_currentMenuType == MT_InfoMenu        ||
            /*m_currentMenuType == MT_SourceMenu  || 
            m_currentMenuType == MT_ControlMenu ||*/
            m_currentMenuType == MT_FxMenu      ||
            m_currentMenuType == MT_OutputMenu
        )
    {
        // select output plane for FX or OUT menu
        planeId = &m_activeOutputPlane.planeId;
        *planeId += diff;
        int minValue = 0;
        int maxValue = m_registry.planes().size() - 1;
        if (*planeId > maxValue) {*planeId = maxValue; return;}
        if (*planeId < minValue) {*planeId = minValue; return;}
        m_activeOutputPlane.planeId = *planeId;


        for (int i = 0; i < MEDIA_SLOT_COUNT; ++i) {
            InputConfig* inputConfig = m_registry.inputMappings().getInputConfig(i, false);
            if (!inputConfig) {
                m_registry.inputMappings().setFocusedMediaSlot(-1);
                continue;
            }

            if (inputConfig->planeId == *planeId) {
                m_registry.inputMappings().setFocusedMediaSlot(i);
                if (m_currentMenuType == MT_SourceMenu) {
                    setMenu(MT_SourceMenu);
                    SelectActiveSourceFolder();
                }
                
                break;
            }
        }
    }

    // if (id != nullptr)
    // {
    //     *id += diff;
    //     int minValue = 0;
    //     int maxValue = m_registry.planes().size() - 1;
    //     if (*id > maxValue) *id = maxValue;
    //     if (*id < minValue) *id = minValue;

    //     if (m_currentMenuType == MT_SourceMenu || 
    //         m_currentMenuType == MT_ControlMenu ) 
    //     {
    //         m_activeOutputPlane.planeId = *id; // switch output plane as well
    //     }
    // }
}

void MenuSystem::goUpHierachy() {
    if (m_currentMenuPath.size() > 1) {
        m_currentMenuPath.pop_back();
        m_ui.setClearFrame(true);
    }
}

 void MenuSystem::appendStateToMenuPath(MenuState menuState)
 {
    m_nextMenuPath.push_back(menuState);
 }

void MenuSystem::handleMenuHierachyNavigation()
{
    // if (m_ui.isNavigationEventTriggered(NavigationEvent::Type::NavigationLeft)) 
    if (m_ui.isNavigationEventTriggered(NavigationEvent::Type::NavigationExit)) 
    {
        goUpHierachy();
    }
}

void MenuSystem::render()
{
    m_ui.NewFrame();

    InputConfig* inputConfig = m_registry.inputMappings().getFocusedInputConfig(true);
    if (inputConfig) 
        m_registry.inputMappings().focusedPlane = inputConfig->planeId;
    else 
        m_registry.inputMappings().focusedPlane = -1;   

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
    for (MenuState menuState : m_nextMenuPath) {
        m_currentMenuPath.push_back(menuState);
    }
    m_nextMenuPath.clear();
}

void MenuSystem::ClearSlot() {
    int slotId = m_registry.inputMappings().getFocusedMediaSlot();
    m_registry.inputMappings().removeConfig(slotId, true);
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
    m_ui.MenuTitleWidget("INFO", TextAlign::CENTER);
    m_ui.MenuTitleWidget(m_registry.inputMappings().focusedMediaButtonName(), TextAlign::LEFT);
    m_ui.PlanePreviewWidget(m_registry.planes(), m_activeOutputPlane.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_SMALL);
    m_ui.NewLine();

    m_ui.PushTranslate(4, 20);
    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);

    InputConfig* currentConfig = m_registry.inputMappings().getFocusedInputConfig();
    if (!currentConfig) {
        m_ui.Label("No input selected");
        m_ui.EndList();
        return;
    }
    
    if (VideoInputConfig* videoInputConfig = dynamic_cast<VideoInputConfig*>(currentConfig)) {
        m_ui.Label("Type: Mediafile");
        std::string fileName = videoInputConfig->fileName;
        int lastSlashPos = fileName.find_last_of('/');
        fileName = fileName.substr(lastSlashPos + 1);
        if (fileName.size() > 20) {
            fileName = fileName.substr(0, 20) + "...";
        }
        m_ui.Label("Name: " + fileName);
        m_ui.Label("Player ID: " + std::to_string(currentConfig->playerId));
        m_ui.Spacer();
        if (m_ui.Action("Show source")) {
            SelectActiveSourceFolder(false);
        }
        if (m_ui.Action("Deactivate")) {
            m_eventBus.publish(PlaneEvent(m_activeOutputPlane.planeId));
        }
        std::string previewFilename = videoInputConfig->fileName + ".preview";
        MediaPreview(previewFilename, glm::uvec2(156, 96));
    }
    else if (HdmiInputConfig* hdmiInputConfig = dynamic_cast<HdmiInputConfig*>(currentConfig)) {
        m_ui.Label("Type: HDMI");
        std::string inputName = "Source: HDMI" + std::to_string(hdmiInputConfig->hdmiPort+1);
        m_ui.Label(inputName);
        m_ui.Label("Player ID: " + std::to_string(currentConfig->playerId));
        m_ui.Spacer();
        if (m_ui.Action("Show source")) {
            SelectActiveSourceFolder(false);
        }
        if (m_ui.Action("Deactivate")) {
            m_eventBus.publish(PlaneEvent(m_activeOutputPlane.planeId));
        }
    }
    else if (ShaderInputConfig* shaderInputConfig = dynamic_cast<ShaderInputConfig*>(currentConfig)) {
        m_ui.Label("Type: Shader");
        std::string shaderName = shaderInputConfig->fileName;
        int lastSlashPos = shaderName.find_last_of('/');
        shaderName = shaderName.substr(lastSlashPos + 1);
        m_ui.Label("Name: " + shaderName);
        m_ui.Label("Player ID: " + std::to_string(currentConfig->playerId));
        m_ui.Spacer();
        if (m_ui.Action("Show source")) {
            SelectActiveSourceFolder(false);
        }
        if (m_ui.Action("Deactivate")) {
            m_eventBus.publish(PlaneEvent(m_activeOutputPlane.planeId));
        }
    }
    
    m_ui.EndList();
    m_ui.PopTranslate();
}

// ##### SOURCE MENU #####
void MenuSystem::SourceMenu() 
{
    m_ui.MenuTitleWidget("SRC", TextAlign::CENTER);
    m_ui.MenuTitleWidget(m_registry.inputMappings().focusedMediaButtonName(), TextAlign::RIGHT);
    // m_ui.PlanePreviewWidget(m_registry.planes(), m_activeOutputPlane.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_SMALL);
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
    // if (!m_currentMenuPath.back().name.empty()) {
    //     path += m_currentMenuPath.back().name + "/";
    // }
    return path;
}

void MenuSystem::MediaPreview(const std::string& filename, glm::uvec2 pos)
{
    const ImageBuffer& previewImage = m_registry.mediaPool().getPreview(filename);
    if(m_preview.imageFileName != filename)
    {
        if (previewImage.isValid) {
            m_preview.imageFileName = filename;
            m_preview.frameIndex = 0;
        }
    }

    if (previewImage.isValid) {
        m_ui.AnimationFrameWidget(previewImage, m_preview.frameIndex, pos);
    }
}

void MenuSystem::SelectActiveSourceFolder(bool staged)
{
    InputConfig *inputConfig = m_registry.inputMappings().getFocusedInputConfig(staged);
    if (!inputConfig) return;
    
    if (VideoInputConfig *videoInputConfig = dynamic_cast<VideoInputConfig *>(inputConfig))
    {
        printf("Filename: %s\n", videoInputConfig->fileName.c_str());
        std::vector<std::string> menuPath = splitPath(videoInputConfig->fileName);
        for (const std::string& pathSegment : menuPath) {
            printf("Path Segment: %s\n", pathSegment.c_str());
        }
        
        //std::string selectedFileName = menuPath.back();
        menuPath.pop_back(); // remove filename
        if (!menuPath.empty() && menuPath.front() == "videos") { // remove first element "videos"
            menuPath.front() = "";
        }
        if(menuPath.size() > 0){
            for (size_t i = 0; i < menuPath.size(); ++i) {
                const std::string &pathElementName = menuPath[i];
                printf("entering %s\n", pathElementName.c_str());
                appendStateToMenuPath(MenuState(0, [this](){FileSelection();}, pathElementName));
            }
            m_focusActiveSource = true;           
        }
        else
        {
            appendStateToMenuPath(MenuState(0, [this](){FileSelection();}));
        }
    }
    else if (/*HdmiInputConfig *hdmiInputConfig = */dynamic_cast<HdmiInputConfig *>(inputConfig))
    {
        appendStateToMenuPath(MenuState(0, [this](){LiveInputSelection();}));
        // int port = hdmiInputConfig->hdmiPort;
    }
    else if (ShaderInputConfig *shaderInputConfig = dynamic_cast<ShaderInputConfig *>(inputConfig))
    {
        printf("Filename: %s\n", shaderInputConfig->fileName.c_str());
        std::vector<std::string> menuPath = splitPath(shaderInputConfig->fileName);
        for (const std::string& pathSegment : menuPath) {
            printf("Path Segment: %s\n", pathSegment.c_str());
        }
        
        menuPath.pop_back(); // remove filename
        if (menuPath.size() >=2 && menuPath[0] == "shaders" && menuPath[1] == "generative") { // remove first element "videos"
            menuPath.pop_back();
            menuPath[0] = "";
        }
        if(menuPath.size() > 0){
            for (size_t i = 0; i < menuPath.size(); ++i) {
                const std::string &pathElementName = menuPath[i];
                printf("entering %s\n", pathElementName.c_str());
                appendStateToMenuPath(MenuState(0, [this](){ShaderSelection();}, pathElementName));
            }
            m_focusActiveSource = true;           
        }
        else
        {
            appendStateToMenuPath(MenuState(0, [this](){ShaderSelection();}));
        }
    }
}

void MenuSystem::FileSelection()
{
    m_ui.MenuTitleWidget("MEDIA", TextAlign::CENTER);
    m_ui.MenuTitleWidget(m_registry.inputMappings().focusedMediaButtonName(), TextAlign::RIGHT);
    // m_ui.PlanePreviewWidget(m_registry.planes(), m_activeOutputPlane.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_SMALL);
    m_ui.NewLine();

    m_ui.PushTranslate(4, 20);
    auto config = std::make_unique<VideoInputConfig>();
    config->looping = m_registry.settings().defaultLooping;

    int slotId = m_registry.inputMappings().getFocusedMediaSlot();
    VideoInputConfig* currentConfig = m_registry.inputMappings().getVideoInputConfig(slotId, true);
    if (currentConfig) {
        *config = *currentConfig;
    }

    std::string videoPath = currentDirectoryPath();
    std::vector<DirectoryEntry> entries = m_registry.mediaPool().getVideoDirectoryEntries(videoPath);
    // printf("VideoPath: %s Entries: %ld\n", videoPath.c_str(), entries.size());
    bool changed = false;
    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
    if(SubMenu("New Folder", [this](){TextInputDialog();}))
    {
        m_textInputDialog.cursorIdx = 0;
        m_textInputDialog.title = "New Folder";
        m_textInputDialog.text = "noname";
        m_textInputDialog.func = [this](){
            std::string newDirectoryPath = m_registry.mediaPool().getVideoFilePath() + currentDirectoryPath() + m_textInputDialog.text;
            printf("Creating new Folder: %s\n", newDirectoryPath.c_str());
            std::filesystem::create_directory(newDirectoryPath);
        };
        m_ui.setClearFrame(false);
    }
    if(m_ui.Action("USB-Drive")) {
        printf("Enter USB-Drive\n");
    }

    m_ui.Spacer();
    if(videoPath!="") m_ui.Text(videoPath);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM_MONOSPACED);
    int fileListStartIdx = m_ui.CurrentListSize();
    for (size_t i = 0; i < entries.size(); ++i) {
        const DirectoryEntry& entry = entries[i];
        if (entry.isDir) {
            SubDir(entry.name, [this]() { FileSelection(); });
        }
        else {
            bool openFileMenu = false;
            std::string entryName = entry.name;
            if (entryName.size() > 20) {
                entryName = entryName.substr(0, 20) + "...";
            }
            if (m_ui.RadioButton(entryName, (config->fileName == entry.absolutePath), &openFileMenu)) {
                config->fileName = entry.absolutePath;
                changed = true;
            }
            // Set focus to this entry if it's the active media
            if (m_focusActiveSource) {
                VideoInputConfig* videoConfig = m_registry.inputMappings().getVideoInputConfig(slotId);
                if (videoConfig && entry.absolutePath == videoConfig->fileName) {
                    m_currentMenuPath.back().fIdx = m_ui.CurrentListSize()-1;
                    m_focusActiveSource = false;
                }
            }
            
            if(openFileMenu) {
                // m_fileManagerDialog.filename = entry.name;
                // m_fileManagerDialog.absolutePath = entry.absolutePath;
                m_fileManagerDialog.slotId = slotId;
                m_fileManagerDialog.entry = entry;
                m_fileManagerDialog.fileType = FileManagerDialogData::FileType::Video;
                m_ui.setClearFrame(false);
                appendStateToMenuPath(MenuState(0, [this](){FileManagerDialog();}));
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
                MediaPreview(previewFilename, glm::uvec2(156, 96));
            }
        }
    }

    if (changed) {
        config->planeId = m_activeOutputPlane.planeId;
        m_registry.inputMappings().stageInputConfig(slotId, std::move(config));
    }
    
    m_ui.PopTranslate();
}

void MenuSystem::LiveInputSelection() 
{
    m_ui.MenuTitleWidget("LIVE", TextAlign::CENTER);
    m_ui.MenuTitleWidget(m_registry.inputMappings().focusedMediaButtonName(), TextAlign::RIGHT);
    // m_ui.PlanePreviewWidget(m_registry.planes(), m_activeOutputPlane.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_SMALL);
    m_ui.NewLine();
    
    m_ui.PushTranslate(4, 20);
    auto config = std::make_unique<HdmiInputConfig>();
    int slotId = m_registry.inputMappings().getFocusedMediaSlot();
    HdmiInputConfig* currentConfig = m_registry.inputMappings().getHdmiInputConfig(slotId, true);
    if (currentConfig) { *config = *currentConfig; }

    bool changed = false;
    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
    if (m_ui.RadioButton("HDMI Input 1", currentConfig && (config->hdmiPort == 0))) {
        config->hdmiPort = 0; 
        changed = true; 
    }
    if (m_ui.RadioButton("HDMI Input 2", currentConfig && (config->hdmiPort == 1))) { 
        config->hdmiPort = 1; 
        changed = true; 
    }
    m_ui.EndList();


    if (changed)  {
        //printf("ID: %d, PORT: %d\n", id, config->hdmiPort);
        config->planeId = m_activeOutputPlane.planeId;
        m_registry.inputMappings().stageInputConfig(m_registry.inputMappings().getFocusedMediaSlot(), std::move(config));
    }
    m_ui.PopTranslate();
}

void MenuSystem::ShaderSelection()
{
    m_ui.MenuTitleWidget("SHADER", TextAlign::CENTER);
    m_ui.MenuTitleWidget(m_registry.inputMappings().focusedMediaButtonName(), TextAlign::RIGHT);
    // m_ui.PlanePreviewWidget(m_registry.planes(), m_activeOutputPlane.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_SMALL);
    m_ui.NewLine();

    m_ui.PushTranslate(4, 20);
    auto config = std::make_unique<ShaderInputConfig>();

    int slotId = m_registry.inputMappings().getFocusedMediaSlot();
    ShaderInputConfig* currentConfig = m_registry.inputMappings().getShaderInputConfig(slotId, true);
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
        else {
            bool openFileMenu = false;
            std::string entryName = entry.name;
            if (entryName.size() > 20) {
                entryName = entryName.substr(0, 20) + "...";
            }

            if (m_ui.RadioButton(entryName, (config->fileName == entry.absolutePath), &openFileMenu)) {
                config->fileName = entry.absolutePath;
                changed = true;
            }
            // Set focus to this entry if it's the active shader
            if (m_focusActiveSource) {
                ShaderInputConfig* shaderInputConfig = m_registry.inputMappings().getShaderInputConfig(m_registry.inputMappings().getFocusedMediaSlot());
                if (shaderInputConfig && entry.absolutePath == shaderInputConfig->fileName) {
                    m_currentMenuPath.back().fIdx = m_ui.CurrentListSize()-1;
                    m_focusActiveSource = false;
                }
            }

            if(openFileMenu) {
                m_fileManagerDialog.slotId = slotId;
                m_fileManagerDialog.entry = entry;
                m_fileManagerDialog.fileType = FileManagerDialogData::FileType::Shader;
                m_ui.setClearFrame(false);
                appendStateToMenuPath(MenuState(0, [this](){FileManagerDialog();}));
            }
        }
    }
    m_ui.EndList(); 

    if (changed) {
        config->planeId = m_activeOutputPlane.planeId;
        m_registry.inputMappings().stageInputConfig(slotId, std::move(config));
    }

    m_ui.PopTranslate();
}

// ##### CONTROL MENU #####
void MenuSystem::ControlMenu() 
{
    m_ui.MenuTitleWidget("CTL", TextAlign::CENTER);
    m_ui.MenuTitleWidget(m_registry.inputMappings().focusedMediaButtonName(), TextAlign::RIGHT);
    // m_ui.PlanePreviewWidget(m_registry.planes(), m_activeOutputPlane.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_SMALL);
    m_ui.NewLine();

    m_ui.PushTranslate(4, 20);
    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);

    InputConfig* currentConfig = m_registry.inputMappings().getFocusedInputConfig(true);

    if (!currentConfig) {
        m_ui.Label("No input selected");
        m_ui.EndList();
        return;
    }
    
    if (VideoInputConfig* videoInputConfig = dynamic_cast<VideoInputConfig*>(currentConfig)) {
        // m_ui.Label("Type: Mediafile");
        // std::string fileName = videoInputConfig->fileName;
        // int lastSlashPos = fileName.find_last_of('/');
        // fileName = fileName.substr(lastSlashPos + 1);
        // if (fileName.size() > 20) {
        //     fileName = fileName.substr(0, 20) + "...";
        // }
        // m_ui.Label("Name: " + fileName);
        m_ui.Spacer(85);
        if (m_ui.CheckBox("loop", videoInputConfig->looping)) { 
            videoInputConfig->looping = !videoInputConfig->looping; 
        }
        m_ui.PlaybackControlWidget(*videoInputConfig);

        // if (m_ui.CheckBox("backwards (N/A)", videoInputConfig->backwards)) {
        //     videoInputConfig->backwards = !videoInputConfig->backwards;
        // }
        // m_ui.Text("speed (N/A)");
        m_ui.Spacer();
        m_ui.SpinBoxInt("Output Plane", videoInputConfig->planeId, 0, 3, 1, {"1", "2", "3", "4"});
        std::string previewFilename = videoInputConfig->fileName + ".preview";
        MediaPreview(previewFilename, glm::uvec2(80, 30));
    }
    else if (HdmiInputConfig* hdmiInputConfig = dynamic_cast<HdmiInputConfig*>(currentConfig)) {
        // m_ui.Label("Type: HDMI");
        std::string inputName = "HDMI Input " + std::to_string(hdmiInputConfig->hdmiPort+1);
        m_ui.Label(inputName);
        m_ui.Spacer();
        m_ui.SpinBoxInt("Output Plane", hdmiInputConfig->planeId, 0, 3, 1, {"1", "2", "3", "4"});
    }
    else if (ShaderInputConfig* shaderInputConfig = dynamic_cast<ShaderInputConfig*>(currentConfig)) {
        // m_ui.Label("Type: Shader");
        std::string shaderName = shaderInputConfig->fileName;
        int lastSlashPos = shaderName.find_last_of('/');
        shaderName = shaderName.substr(lastSlashPos + 1);
        m_ui.Label(shaderName);
        m_ui.Spacer();

        for (auto& kv : shaderInputConfig->shaderConfig.params) {
            auto& param = kv.second;
            if (std::holds_alternative<IntParameter>(param)) {
                auto& intParam = std::get<IntParameter>(param);  
                m_ui.SpinBoxInt(intParam.name, intParam.value, intParam.min, intParam.max, intParam.step);
            } 
            else if (std::holds_alternative<FloatParameter>(param)) {
                auto& floatParam = std::get<FloatParameter>(param); 
                m_ui.SpinBoxFloat(floatParam.name, floatParam.value, floatParam.min, floatParam.max, floatParam.step);
            }
            else if (std::holds_alternative<Vec2Parameter>(param)) {
                auto& vec2Param = std::get<Vec2Parameter>(param); 
                m_ui.SpinBoxVec2(vec2Param.name, vec2Param.value, vec2Param.min, vec2Param.max, vec2Param.step);
            }
        }
        m_ui.Spacer();
        m_ui.SpinBoxInt("Output Plane", shaderInputConfig->planeId, 0, 3, 1, {"1", "2", "3", "4"});
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

    m_ui.PlanePreviewWidget(m_registry.planes(), m_activeOutputPlane.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_SMALL);
    m_ui.NewLine();

    m_ui.PushTranslate(0, 20);

    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    // m_ui.Label("---Build In FX---");
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
    m_ui.Spacer();
    // m_ui.Label("---Custom FX---");
    SubMenu("Add Custom FX", [this](){ CustomEffectShaderSelection(); });
    if(plane.shaderConfig.groups.size() > 2) {  // todo: '2' is temp value for the two build-in effects (chroma key, color correction)
        if (m_ui.Action("Clear Custom FX")) {
            plane.extShaderFilename = "";
            m_eventBus.publish(EffectShaderEvent(m_activeOutputPlane.planeId));
        }
    }
    m_ui.EndList();
    m_ui.PopTranslate();
}

void MenuSystem::CustomEffectShaderSelection()
{
    m_ui.MenuTitleWidget("CUSTOM FX", TextAlign::CENTER);
    m_ui.NewLine();

    m_ui.PlanePreviewWidget(m_registry.planes(), m_activeOutputPlane.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_SMALL);
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
    m_ui.MenuTitleWidget(m_effectName, TextAlign::CENTER);
    m_ui.NewLine();
    m_ui.PlanePreviewWidget(m_registry.planes(), m_activeOutputPlane.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_SMALL);
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
        } 
        else if (std::holds_alternative<FloatParameter>(param)) {
            auto& floatParam = std::get<FloatParameter>(param); 
            m_ui.SpinBoxFloat(floatParam.name, floatParam.value, floatParam.min, floatParam.max, floatParam.step);
        }
        else if (std::holds_alternative<Vec2Parameter>(param)) {
            auto& vec2Param = std::get<Vec2Parameter>(param); 
            glm::vec2 value(vec2Param.value.x, vec2Param.value.y);
            glm::vec2 min(vec2Param.min.x, vec2Param.min.y);
            glm::vec2 max(vec2Param.max.x, vec2Param.max.y);
            glm::vec2 step(vec2Param.step.x, vec2Param.step.y);
            m_ui.SpinBoxVec2(vec2Param.name, vec2Param.value, vec2Param.min, vec2Param.max, vec2Param.step);
        } 
    }
    m_ui.Spacer();
    if(m_ui.Action("Reset"))
    {
        for (const auto& paramName : group) {
            if (!shaderConfig.params.contains(paramName)) continue;
            auto& param = shaderConfig.params[paramName];
            if (std::holds_alternative<IntParameter>(param)) {
                auto& intParam = std::get<IntParameter>(param);
                intParam.reset();
            } 
            else if (std::holds_alternative<FloatParameter>(param)) {
                auto& floatParam = std::get<FloatParameter>(param); 
                floatParam.reset();
            }
            else if (std::holds_alternative<Vec2Parameter>(param)) {
                auto& vec2Param = std::get<Vec2Parameter>(param);
                vec2Param.reset();
            } 
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
    m_ui.PlanePreviewWidget(m_registry.planes(), m_activeOutputPlane.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_SMALL);
    m_ui.NewLine();
    m_ui.PushTranslate(0, 20);

    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    // SubMenu("Mrs. Mask", [this](){ Mask(); });
    SubMenu("Projection Mapping", [this](){ Mapping(); });
    m_ui.Spacer();
    m_ui.SpinBoxInt("Blend Mode", (int&)m_registry.planes()[m_activeOutputPlane.planeId].blendMode, 0, 2, 1, {"Normal", "Multiply", "Add"});
    m_ui.SpinBoxFloat("Opacity", m_registry.planes()[m_activeOutputPlane.planeId].opacity, 0.0f, 1.0f);
    // if(m_ui.CheckBox("Use Fader For Opacity", m_registry.planes()[m_activeOutputPlane.planeId].useFaderForOpacity))
    // {
    //     m_registry.planes()[m_activeOutputPlane.planeId].useFaderForOpacity = !m_registry.planes()[m_activeOutputPlane.planeId].useFaderForOpacity;
    // }
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
    m_ui.MenuTitleWidget("PROJECTION MAPPING", TextAlign::CENTER);
    m_ui.NewLine();

    m_activeOutputPlane.selectedVertexId =  m_currentMenuPath.back().fIdx - m_ui.CurrentListSize();
    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.PlanePreviewWidget(m_registry.planes(), m_activeOutputPlane.planeId, UI::PlanePreviewStyle::PLANE_PREVIEW_VERTICES);
    m_ui.NewLine();
    m_ui.PushTranslate(0, 20);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
    m_ui.SpinBoxFloat("Scale", m_registry.planes()[m_activeOutputPlane.planeId].scale, 0.0f, 10.0f, 0.1f);
    m_ui.SpinBoxVec2("Translation", m_registry.planes()[m_activeOutputPlane.planeId].translation, glm::vec2(-2.0f, -2.0f), glm::vec2(2.0f, 2.0f), glm::vec2(0.1f, 0.1f));
    m_ui.SpinBoxInt("Output", m_registry.planes()[m_activeOutputPlane.planeId].hdmiId, 0, 1, 1, {"HDMI 1", "HDMI 2"});
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
    m_ui.PushTranslate(0, 20);

    std::string eth0;
    std::string wlan0;
    NetworkTools::getIPAddress("eth0", eth0);
    NetworkTools::getIPAddress("wlan0", wlan0);

    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
    SubMenu("Wifi QR-Code", [this](){ WifiQrCode(); });
    SubMenu("File Manager QR-Code", [this](){ TFMQrCode(); });
    m_ui.Spacer();
    if (!eth0.empty()) m_ui.Label("Eth. IP: " + eth0);
    if (!wlan0.empty()) m_ui.Label("Wifi IP: " + wlan0);
    //m_ui.Label("SSID: VM-1");
    //m_ui.Label("Pass: vmone12345");
    m_ui.EndList();
        

    m_ui.PopTranslate();
}

void MenuSystem::WifiQrCode()
{
    m_ui.MenuTitleWidget("WiFi", TextAlign::CENTER);
    m_ui.NewLine();
    const ImageBuffer& imageBuffer = m_registry.mediaPool().getQrCodeImageBuffer();
    if (imageBuffer.isValid) {
        m_ui.Image(imageBuffer, glm::uvec2(105, 65-10));
    }
    m_ui.PushTranslate(0, 170-10);
    BDF::TextStyle textStyle = BDF::TEXTSTYLE::MENU_ITEM;
    textStyle.align = TextAlign::CENTER;
    m_ui.TextStyle(textStyle);
    m_ui.Label(m_registry.settings().apCredentials.ssid + " / " + m_registry.settings().apCredentials.psk);
    m_ui.PopTranslate();
}

void MenuSystem::TFMQrCode()
{
    m_ui.MenuTitleWidget("File Manager", TextAlign::CENTER);
    const ImageBuffer& imageBuffer = m_registry.mediaPool().getQrCodeTFMImageBuffer();
    if (imageBuffer.isValid) {
        m_ui.Image(imageBuffer, glm::uvec2(105, 65));
    }
}

// GLOBAL SETTINGS MENU
void MenuSystem::SettingsMenu() 
{
    m_ui.MenuTitleWidget("Settings", TextAlign::CENTER);
    m_ui.NewLine();
    m_ui.PushTranslate(0, 20);

    /*
    Menu Structure
    
    - Fade Time
    - Volume
    - Connected Devices / Hardware Setup
        - "Scan for New" -> "Initialize"
        - Screen Rotation
        - Info 
    - Network
        - Show WiFiQR
        - Show FileManagerQR
        - IP-Adresses
    - About
        - Version (Basic/Pro)
    */

    Settings& settings = m_registry.settings();

    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
    m_ui.SpinBoxInt("Fade Time", settings.fadeTime, 0, 10);
    m_ui.SpinBoxInt("Volume", settings.volume, 0, 10);
    if (m_ui.CheckBox("Show Dev UI", settings.showUI)) { settings.showUI = !settings.showUI; };
    m_ui.Spacer();
    SubMenu("Connected Devices", [this](){ HardwareSetupMenu(); });
    if(!m_registry.settings().kiosk.enabled) {
        SubMenu("Network", [this](){ NetworkMenu(); });
    }
    m_ui.Spacer();
    SubMenu("About", [this](){ AboutMenu(); });
    
    // std::string useFader = "Use Fader (" + std::to_string(settings.analog0).substr(0, 4) + ")";
    // if(m_ui.CheckBox(useFader, settings.useFader)) { settings.useFader = !settings.useFader; }
    // std::string useRotaryAsFader = "Use Rotary (" + std::to_string(settings.rotary) + ")" ;
    // if(m_ui.CheckBox(useRotaryAsFader, settings.useRotaryAsFader)) { settings.useRotaryAsFader = !settings.useRotaryAsFader; }
    // if (m_ui.CheckBox("Use UVC", settings.useUvcCaptureDevice)) { settings.useUvcCaptureDevice = !settings.useUvcCaptureDevice; };
    // if (m_registry.settings().isProVersion) { m_ui.SpinBoxInt("Rot. Sensit.", settings.rotarySensitivity, 1, 20) };
    // if (m_ui.CheckBox("Default Looping", settings.defaultLooping)) { settings.defaultLooping = !settings.defaultLooping; };
    // if (m_ui.CheckBox("Show UI", settings.showUI)) { settings.showUI = !settings.showUI; };

    m_ui.EndList();

    m_ui.PopTranslate();
}

void MenuSystem::AboutMenu()
{
    Settings& settings = m_registry.settings();

    m_ui.MenuTitleWidget("About", TextAlign::CENTER);
    m_ui.NewLine();
    m_ui.PushTranslate(0, 20);
    m_ui.BeginList(&m_currentMenuPath.back().fIdx);

    std::string versionPostfix = " Basic";
    if (settings.isProVersion) versionPostfix = " Pro";
    m_ui.Label("Version: " + VERSION + versionPostfix);
    // if (m_ui.CheckBox("Is Pro Version", settings.isProVersion)) { settings.isProVersion = !settings.isProVersion; };


    m_ui.EndList();
    m_ui.PopTranslate();

}

// ##### HARDWARE SETUP MENU #####
void MenuSystem::HardwareSetupMenu() 
{  
    Settings& settings = m_registry.settings();

    m_ui.MenuTitleWidget("Connected Devices", TextAlign::CENTER);
    m_ui.NewLine();
    m_ui.PushTranslate(0, 20);

    m_ui.BeginList(&m_currentMenuPath.back().fIdx);
    m_ui.TextStyle(BDF::TEXTSTYLE::MENU_ITEM);
    if (m_registry.settings().isHdmiOutputReady && m_registry.settings().isHdmiInputReady) {
        if (m_ui.Action("Re-Initialize")) {
            m_registry.settings().isHdmiOutputReady = false;
            m_registry.settings().isHdmiInputReady = false;
            m_eventBus.publish(SystemEvent(SystemEvent::Type::Restart));  
        }
        m_ui.SpinBoxInt("Screen Rotation", (int&)settings.hdmiRotation0, 0, 3, 1, {"0 Deg", "90 Deg", "180 Deg", "270 Deg"});
    }
    else {
        m_ui.Label("Scanning...");
    }
    
    m_ui.Spacer();

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
    m_ui.PopTranslate();
}

// ##### KEYS/MATRIX MENU #####
// void MenuSystem::ButtonMatrixMenu() 
// {
//     m_ui.MenuTitleWidget("Button");
//     m_ui.NewLine();

//     int bank = m_registry.inputMappings().focusedBank;
//     for (size_t i = 0; i < m_buttonTexts.size(); ++i) {
//         int mediaSlot = bank * 16 + int(i);
//         InputConfig* currentConfig = m_registry.inputMappings().getInputConfig(mediaSlot);
//         if (!currentConfig) {
//             m_buttonTexts[i].second = COLOR::BLACK;
//         }

//         if (VideoInputConfig* videoInputConfig = dynamic_cast<VideoInputConfig*>(currentConfig)) {
//             if (videoInputConfig->isActive) m_buttonTexts[i].second = COLOR::RED;
//             else m_buttonTexts[i].second = COLOR::DARK_RED;
//         }
//         else if (HdmiInputConfig* hdmiInputConfig = dynamic_cast<HdmiInputConfig*>(currentConfig)) {
//             if (hdmiInputConfig->isActive) m_buttonTexts[i].second = COLOR::BLUE;
//             else m_buttonTexts[i].second = COLOR::DARK_BLUE;
//         }
//         else if (ShaderInputConfig* shaderInputConfig = dynamic_cast<ShaderInputConfig*>(currentConfig)) {
//             if (shaderInputConfig->isActive) m_buttonTexts[i].second = COLOR::GREEN;
//             else m_buttonTexts[i].second = COLOR::DARK_GREEN;
//         }
//     }
//     m_ui.ButtonMatrixWidget(m_buttonTexts);
// }
