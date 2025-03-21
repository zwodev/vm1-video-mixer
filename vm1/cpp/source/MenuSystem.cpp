#include "MenuSystem.h"
#include "UIHelper.h"

#include <imgui.h>
#include <vector>
#include <string>

MenuSystem::MenuSystem(Registry &registry) : m_registry(registry) {
    buildMenuStructure();
}

void MenuSystem::buildMenuStructure() {
    rootMenu = std::make_unique<SubmenuEntry>("root");
    currentMenu = rootMenu.get();
    currentSelection = 0;

    // input association menu
    auto inputMenu = std::make_unique<SubmenuEntry>("input");
    buildInputMenuStructure(inputMenu);
    rootMenu->addSubmenuEntry(std::move(inputMenu));
}

void MenuSystem::buildInputMenuStructure(std::unique_ptr<SubmenuEntry>& rootEntry) {
    const int bankCount = 4;
    const int mediaButtonsCount = 16;

    // menu entries level 0 ("bank-selection")
    for (int i = 0; i < bankCount; i++)
    {
        auto menuBankLevel = std::make_unique<SubmenuEntry>("bank-" + std::to_string(i));

        // menu entries level 1 ("media-selection")
        for (int j = 0; j < mediaButtonsCount; j++)
        {
            auto mediaButtonEntry = std::make_unique<SubmenuEntry>("media-" + std::to_string(j));

            // file
            auto fileSelectionEntry = std::make_unique<FilesystemEntry>("files", "../videos/", j + i * mediaButtonsCount);
            mediaButtonEntry->addSubmenuEntry(std::move(fileSelectionEntry));

            // live
            std::unique_ptr<MenuEntry> liveSelectionEntry = std::make_unique<SubmenuEntry>("live");
            mediaButtonEntry->addSubmenuEntry(std::move(liveSelectionEntry));

            // shader
            std::unique_ptr<MenuEntry> shaderSelectionEntry = std::make_unique<SubmenuEntry>("shader");
            mediaButtonEntry->addSubmenuEntry(std::move(shaderSelectionEntry));

            menuBankLevel->addSubmenuEntry(std::move(mediaButtonEntry));
        }

        rootEntry->addSubmenuEntry(std::move(menuBankLevel));
    }
}

void MenuSystem::render()
{
    // ImGui::Begin("Menu System");
    SubmenuEntry *submenuEntry = dynamic_cast<SubmenuEntry *>(currentMenu);

    // Handle left arrow key (go back)
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) && currentMenu != rootMenu.get())
    {
        SubmenuEntry *parentEntry = static_cast<SubmenuEntry *>(currentMenu->parentEntry);
        currentSelection = 0;
        for (int i = 0; i < parentEntry->submenus.size(); i++)
        {
            if (parentEntry->submenus[i].get() == currentMenu)
            {
                currentSelection = i;
                break;
            }
        }
        currentMenu = currentMenu->parentEntry;
    }

    // Handle right arrow key (go forward)
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
    {
        if (submenuEntry)
        {
            previousMenu = currentMenu;
            MenuEntry *nextMenuEntry = submenuEntry->submenus[currentSelection].get();
            if (dynamic_cast<SubmenuEntry *>(nextMenuEntry))
            {
                currentMenu = nextMenuEntry;
                currentSelection = 0;
            }
            else if (ButtonEntry *buttonEntry = dynamic_cast<ButtonEntry *>(nextMenuEntry))
            {
                printf("Execute Button!\n");
                printf("Displayname: %s\n", buttonEntry->displayName.c_str());
                buttonEntry->action();
            }
            currentMenu->process(m_registry);
        }
    }

    if (!submenuEntry)
        return;

    // Handle up and down arrow keys for item selection
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && currentSelection > 0)
    {
        currentSelection--;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && currentSelection < submenuEntry->submenus.size() - 1)
    {
        currentSelection++;
    }

    for (size_t i = 0; i < submenuEntry->submenus.size(); ++i)
    {
        bool isSelected = (i == currentSelection);

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

    UI::renderOverlayText(std::to_string(currentSelection));
    // ImGui::End();
}
