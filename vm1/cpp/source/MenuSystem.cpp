#include "MenuSystem.h"

#include <imgui.h>
#include <vector>
#include <string>

    MenuSystem::MenuSystem(Registry& registry) : 
        m_registry(registry),
        currentMenu(0)
        
    {
        // root menu
        rootMenu = new SubmenuEntry("root");
        currentMenu = rootMenu;
        currentSelection = 0;
        const int bankCount = 4;
        const int mediaButtonsCount = 16;
        const std::vector<std::string> inputSelection = {"live", "shader"};

        // menu entries level 0 ("bank-selection")
        for(int i = 0; i < bankCount; i++){
            SubmenuEntry* menuLevel0 = new SubmenuEntry("bank-" + std::to_string(i));            
            rootMenu->addSubmenuEntry(menuLevel0);
            
            // menu entries level 1 ("media-selection")
            for (int j = 0; j < mediaButtonsCount; j++) {
                SubmenuEntry*  menuLevel1 = new SubmenuEntry("media-" + std::to_string(j));
                menuLevel0->addSubmenuEntry(menuLevel1);
                // menu entries level 1 ("live/file/shader-selection")
                FilesystemEntry* fileSelection = new FilesystemEntry("files", "../videos/", j + i * mediaButtonsCount);
                menuLevel1->addSubmenuEntry(fileSelection);

                for(int k = 0; k < inputSelection.size(); k++) {
                    SubmenuEntry* menuLevel2 = new SubmenuEntry(inputSelection[k]);
                    menuLevel1->addSubmenuEntry(menuLevel2);
                }
            }
        }  
        
    }

    void  MenuSystem::ColoredText(const std::string& label, ImVec4 textColor, ImVec4 bgColor) {
        ImVec2 textSize = ImGui::CalcTextSize(label.c_str());  // Get text dimensions
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();        // Get screen position of the cursor
    
        ImGui::GetWindowDrawList()->AddRectFilled(
            cursorPos, 
            ImVec2(cursorPos.x + textSize.x, cursorPos.y + textSize.y), 
            ImGui::ColorConvertFloat4ToU32(bgColor) // Convert to ImU32
        );
    
        ImGui::PushStyleColor(ImGuiCol_Text, textColor);
        ImGui::Text("%s", label.c_str());
        ImGui::PopStyleColor();
    }

    void MenuSystem::render() {
        //ImGui::Begin("Menu System");
        
        // Handle left arrow key (go back)
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) && currentMenu != rootMenu) {
            SubmenuEntry *parentEntry = currentMenu->parentEntry;
            currentSelection = 0;
            for(int i = 0; i < parentEntry->submenus.size(); i++) {
                if (parentEntry->submenus[i] == currentMenu){
                    currentSelection = i;
                    break;
                }
            }
            currentMenu = currentMenu->parentEntry;
        }
        
        // Handle right arrow key (go forward)
        SubmenuEntry* submenuEntry = dynamic_cast<SubmenuEntry*>(currentMenu);
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
            if(submenuEntry) {
                previousMenu = currentMenu;  
                MenuEntry* nextMenuEntry = submenuEntry->submenus[currentSelection];
                if (dynamic_cast<SubmenuEntry*>(nextMenuEntry)) {
                    currentMenu = nextMenuEntry;
                }
                else if (ButtonEntry* buttonEntry = dynamic_cast<ButtonEntry*>(nextMenuEntry)) {
                    printf("Execute Button!\n");
                    printf("Displayname: %s\n", buttonEntry->displayName.c_str());
                    buttonEntry->action();
     
                }
                currentMenu->process(m_registry);
                currentSelection = 0;
            }
        }

        if (!submenuEntry)
            return;

        // Handle up and down arrow keys for item selection
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && currentSelection > 0) {
            currentSelection--;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && currentSelection < submenuEntry->submenus.size() - 1) {
            currentSelection++;
        }

        for (size_t i = 0; i < submenuEntry->submenus.size(); ++i) {
            bool isSelected = (i == currentSelection);

            ImGuiStyle& style = ImGui::GetStyle();
    
            std::string label = submenuEntry->submenus[i]->displayName;
            if (isSelected)
            {
                ColoredText(label, ImVec4(0.0f, 0.0f, 0.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // White text on blue background
                ImGui::SetScrollHereY(0.8f);
            }
            else {
                ImGui::Text("%s", label.c_str());
            }
        }
        //ImGui::End();
    }
