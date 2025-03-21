#include "MenuSystem.h"

#include <imgui.h>
#include <vector>
#include <string>

MenuSystem::MenuSystem(Registry &registry) : m_registry(registry),
                                             currentMenu(0)

{
    // root menu
    rootMenu = std::make_unique<SubmenuEntry>("root");
    currentMenu = rootMenu.get();
    currentSelection = 0;

    const int bankCount = 4;
    const int mediaButtonsCount = 16;
    const std::vector<std::string> inputSelection = {"live", "shader"};

    // menu entries level 0 ("bank-selection")
    for (int i = 0; i < bankCount; i++)
    {
        auto menuLevel0 = std::make_unique<SubmenuEntry>("bank-" + std::to_string(i));

        // menu entries level 1 ("media-selection")
        for (int j = 0; j < mediaButtonsCount; j++)
        {
            auto menuLevel1 = std::make_unique<SubmenuEntry>("media-" + std::to_string(j));

            // menu entries level 1 ("live/file/shader-selection")
            auto fileSelection = std::make_unique<FilesystemEntry>("files", "../videos/", j + i * mediaButtonsCount);
            menuLevel1->addSubmenuEntry(std::move(fileSelection));

            for (int k = 0; k < inputSelection.size(); k++)
            {
                std::unique_ptr<MenuEntry> menuLevel2 = std::make_unique<SubmenuEntry>(inputSelection[k]);
                menuLevel1->addSubmenuEntry(std::move(menuLevel2));
            }

            menuLevel0->addSubmenuEntry(std::move(menuLevel1));
        }

        rootMenu->addSubmenuEntry(std::move(menuLevel0));
    }
}

void MenuSystem::SetTextSettings(TextState state, ImVec4 &textColor, ImVec4 &bgColor)
{
    const ImVec4 black = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    const ImVec4 grey = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    const ImVec4 white = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    const ImVec4 red = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    const ImVec4 orange = ImVec4(1.0f, 0.5f, 0.1f, 1.0f);
    const ImVec4 yellow = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);

    switch (state)
    {
    case DEFAULT:
        textColor = white;
        bgColor = black;
        break;
    case SELECTED:
        textColor = black;
        bgColor = white;
        ImGui::SetScrollHereY(0.8f);
        break;
    case HIGHLIGHT:
        textColor = yellow;
        bgColor = black;
        break;
    case ERROR:
        textColor = red;
        bgColor = black;
        break;
    case WARNING:
        textColor = orange;
        bgColor = black;
        break;
    default:
        textColor = grey;
        bgColor = black;
        break;
    }
}

void MenuSystem::RenderText(const std::string &label, TextState textState)
{
    ImVec4 textColor;
    ImVec4 bgColor;
    SetTextSettings(textState, textColor, bgColor);
    int padding_x = 2;
    ImVec2 textSize = ImGui::CalcTextSize(label.c_str()); // Get text dimensions
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();       // Get screen position of the cursor

    ImGui::GetWindowDrawList()->AddRectFilled(
        ImVec2(cursorPos.x - padding_x, cursorPos.y),
        ImVec2(cursorPos.x + textSize.x + padding_x * 2, cursorPos.y + textSize.y),
        ImGui::ColorConvertFloat4ToU32(bgColor) // Convert to ImU32
    );

    ImGui::PushStyleColor(ImGuiCol_Text, textColor);
    ImGui::SetCursorPosX(cursorPos.x + padding_x);
    ImGui::Text("%s", label.c_str());
    ImGui::PopStyleColor();
}

void MenuSystem::RenderOverlayText(const std::string &text)
{
    // ImGui::PushFont(myFont);

    ImDrawList *drawList = ImGui::GetForegroundDrawList();
    drawList->AddText(ImVec2(100, 10), IM_COL32(255, 255, 255, 255), text.c_str());

    // ImGui::SetNextItemAllowOverlap();
    // ImGui::SetCursorPos(ImVec2(100, 100));
    // ImGui::Text("%s", text.c_str());

    // ImGui::PopFont();
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

        if (isSelected)
        {
            RenderText(label, TextState::SELECTED);
        }
        else
        {
            RenderText(label);
        }
    }

    RenderOverlayText(std::to_string(currentSelection));
    // ImGui::End();
}
