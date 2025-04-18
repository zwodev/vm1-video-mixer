#include "MenuSystem.h"
#include "UIHelper.h"

#include <imgui.h>
#include <vector>
#include <string>

const int NUM_BANKS = 4;

MenuSystem::MenuSystem(Registry &registry) : m_registry(registry)
{
    buildMenuStructure();
}

void MenuSystem::buildMenuStructure()
{
    // info menu / info screen
    auto infoMenu = std::make_unique<SubmenuEntry>("Info");
    buildInfoMenuStructure(infoMenu);
    m_menus[MT_InfoSelection] = std::move(infoMenu);

    // input association menu
    auto inputMenu = std::make_unique<SubmenuEntry>("Source");
    buildInputMenuStructure(inputMenu);
    m_menus[MT_InputSelection] = std::move(inputMenu);

    // playback menu
    auto playbackMenu = std::make_unique<SubmenuEntry>("Playback");
    buildPlaybackMenuStructure(playbackMenu);
    m_menus[MT_PlaybackSelection] = std::move(playbackMenu);

    // settings menu
    auto settingsMMenu = std::make_unique<SubmenuEntry>("Settings");
    buildSettingsMenuStructure(settingsMMenu);
    m_menus[MT_SettingsSelection] = std::move(settingsMMenu);

    m_currentActiveMenu = MT_InputSelection;
}

void MenuSystem::buildInfoMenuStructure(std::unique_ptr<SubmenuEntry> &rootEntry)
{
}

void MenuSystem::buildSettingsMenuStructure(std::unique_ptr<SubmenuEntry> &rootEntry)
{
    // show desktop gui
    std::unique_ptr<MenuEntry> showGuiEntry = std::make_unique<SubmenuEntry>("show gui");
    rootEntry->addSubmenuEntry(std::move(showGuiEntry));
}

void MenuSystem::buildInputMenuStructure(std::unique_ptr<SubmenuEntry> &rootEntry)
{
    // file
    auto fileSelectionEntry = std::make_unique<FilesystemEntry>("files", "../videos/");
    rootEntry->addSubmenuEntry(std::move(fileSelectionEntry));

    // live
    std::unique_ptr<MenuEntry> liveSelectionEntry = std::make_unique<SubmenuEntry>("live");
    rootEntry->addSubmenuEntry(std::move(liveSelectionEntry));

    // shader
    std::unique_ptr<MenuEntry> shaderSelectionEntry = std::make_unique<SubmenuEntry>("shader");
    rootEntry->addSubmenuEntry(std::move(shaderSelectionEntry));
}

void MenuSystem::buildPlaybackMenuStructure(std::unique_ptr<SubmenuEntry> &rootEntry)
{
    /*
    Playback-Menu (contains also trigger-options):
    - start-time
    - end-time
    - loop yes/no
    - play backwards
    - live-looper
    - mute audio
    - OSC-In/Out
    - MIDI-In/Out
    - MQTT-In/Out
    */

    std::unique_ptr<MenuEntry> starttimeEntry = std::make_unique<SubmenuEntry>("start-time");
    rootEntry->addSubmenuEntry(std::move(starttimeEntry));

    std::unique_ptr<MenuEntry> endtimeEntry = std::make_unique<SubmenuEntry>("end-time");
    rootEntry->addSubmenuEntry(std::move(endtimeEntry));

    std::unique_ptr<MenuEntry> loopEntry = std::make_unique<SubmenuEntry>("loop");
    rootEntry->addSubmenuEntry(std::move(loopEntry));

    std::unique_ptr<MenuEntry> playbackwardsEntry = std::make_unique<SubmenuEntry>("play backwards");
    rootEntry->addSubmenuEntry(std::move(playbackwardsEntry));

    // ... etc ...
}

void MenuSystem::setMenu(MenuType menuType)
{
    if (m_menus.find(menuType) != m_menus.end())
    {
        m_rootMenu = m_menus[menuType].get();
        m_currentMenu = m_rootMenu;
    }
}

void MenuSystem::handleKeyboardShortcuts()
{
    int numButtons = m_keyboardShortcuts.size();

    // check the media-buttons
    for (int i = 0; i < numButtons; ++i)
    {
        ImGuiKey key = m_keyboardShortcuts[i];
        // if (ImGui::IsKeyDown(key) && ImGui::IsKeyDown(ImGuiKey_LeftShift))
        if (ImGui::IsKeyDown(key))
        {
            int id = m_bank * numButtons + i;
            m_id = id;
            setMenu(m_currentActiveMenu);
            // printf("Current Active Menu: %d\n", m_currentActiveMenu);
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
                m_currentActiveMenu = MT_InfoSelection;
                break;
            case ImGuiKey_W:
                m_currentActiveMenu = MT_InputSelection;
                break;
            case ImGuiKey_E:
                m_currentActiveMenu = MT_PlaybackSelection;
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
                m_currentActiveMenu = MT_SettingsSelection;
                break;
            default:
                break;
            }
            setMenu(m_currentActiveMenu);
        }
    }
}

void MenuSystem::handleMenuNavigationKeys(SubmenuEntry *submenuEntry)
{
    // Handle left arrow key (go back)
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || (ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_UpArrow)))
    {
        if (m_currentMenu == m_rootMenu)
        {
            m_rootMenu = nullptr;
            m_currentMenu = nullptr;
            m_previousMenu = nullptr;
            return;
        }

        SubmenuEntry *parentEntry = static_cast<SubmenuEntry *>(m_currentMenu->parentEntry);
        m_currentSelection = 0;
        for (int i = 0; i < parentEntry->submenus.size(); i++)
        {
            if (parentEntry->submenus[i].get() == m_currentMenu)
            {
                m_currentSelection = i;
                break;
            }
        }
        m_currentMenu = m_currentMenu->parentEntry;
    }

    // Handle right arrow key (go forward)
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) || (ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_DownArrow)))
    {
        if (submenuEntry)
        {
            if (submenuEntry->submenus.size() == 0)
                return;

            m_previousMenu = m_currentMenu;
            MenuEntry *nextMenuEntry = submenuEntry->submenus[m_currentSelection].get();
            if (dynamic_cast<SubmenuEntry *>(nextMenuEntry))
            {
                m_currentMenu = nextMenuEntry;
                m_currentSelection = 0;
            }
            else if (ButtonEntry *buttonEntry = dynamic_cast<ButtonEntry *>(nextMenuEntry))
            {
                buttonEntry->action();
            }

            if (FilesystemEntry *filesystemEntry = dynamic_cast<FilesystemEntry *>(m_currentMenu))
            {
                filesystemEntry->update(m_registry, m_id);
            }
            // currentMenu->process(m_registry);
        }
    }

    if (!submenuEntry)
        return;

    // Handle up and down arrow keys for item selection
    if ((!ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_UpArrow)) && m_currentSelection > 0)
    {
        m_currentSelection--;
    }

    if ((!ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_DownArrow)) && m_currentSelection < submenuEntry->submenus.size() - 1)
    {
        m_currentSelection++;
    }
}

void MenuSystem::renderMenuItems(SubmenuEntry *submenuEntry)
{
    for (size_t i = 0; i < submenuEntry->submenus.size(); ++i)
    {
        bool isSelected = (i == m_currentSelection);

        ImGuiStyle &style = ImGui::GetStyle();

        std::unique_ptr<MenuEntry> &menuEntry = submenuEntry->submenus[i];
        std::string label = menuEntry->displayName;

        if (ButtonEntry *buttonEntry = dynamic_cast<ButtonEntry *>(menuEntry.get()))
        {
            std::string checkBox = buttonEntry->isChecked ? "[*]" : "[ ]";
            label = checkBox + " " + label;
        }

        UI::renderText(label, isSelected ? UI::TextState::SELECTED : UI::TextState::DEFAULT);
    }
}

void MenuSystem::render()
{
    handleKeyboardShortcuts();

    SubmenuEntry *submenuEntry = dynamic_cast<SubmenuEntry *>(m_currentMenu);

    if (!m_rootMenu)
    {
        UI::renderCenteredText("VM-1");
        return;
    }

    // ImGui::Begin("Menu System");

    if (m_currentActiveMenu == MenuType::MT_InfoSelection)
    {
        // printf("Display Information\n");
        std::string filename = m_registry.getValue(m_id);
        UI::renderInfoScreen(m_bank, m_id, filename);
    }
    else
    {
        handleMenuNavigationKeys(submenuEntry);

        ImGui::SetCursorPosY(23);
        renderMenuItems(submenuEntry);
    }

    UI::renderMenuTitle(submenuEntry->displayName);
    UI::renderMediaButtonID(m_id + 1);

    // ImGui::End();
}
