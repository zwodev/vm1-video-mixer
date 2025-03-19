#include "MenuTest.h"

#include <imgui.h>
#include <vector>
#include <string>

    MenuSystem::MenuSystem() : currentMenu(0) {
        menus = {
            {"Menu 1", "Menu 2"},
            {"Submenu 1", "Submenu 2"},
            {"Option 1", "Option 2", "Option 3"}
        };
    }

    void MenuSystem::render() {
        //ImGui::Begin("Menu System");

        // Handle left arrow key (go back)
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) && currentMenu > 0) {
            currentMenu--;
            currentSelection = 0;
        }

        // Handle right arrow key (go forward)
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) && currentMenu < menus.size() - 1) {
            currentMenu++;
            currentSelection = 0;
        }

        for (size_t i = 0; i < menus[currentMenu].size(); ++i) {
            bool is_selected = (i == currentSelection);
            if (ImGui::Selectable(menus[currentMenu][i].c_str(), is_selected)) {
                currentSelection = i;
                if (currentMenu < menus.size() - 1) {
                    currentMenu++;
                    currentSelection = 0;
                }
            }

            // Handle up and down arrow keys for item selection
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && currentSelection > 0) {
                currentSelection--;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && currentSelection < menus[currentMenu].size() - 1) {
                currentSelection++;
            }
        }

        //ImGui::End();
    }
