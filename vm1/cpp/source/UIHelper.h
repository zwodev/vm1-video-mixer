#pragma once

#include "imgui.h"
#include "FontManager.h"
#include "Registry.h"

#include <string>
#include <algorithm>

namespace UI
{
    struct MenuItem {
        std::string label;
        std::vector<MenuItem> children;
        void (*renderFunc)(Registry*, int) = nullptr;
    };

    enum TextState
    {
        DEFAULT,
        HIGHLIGHT,
        SELECTED,
        ERROR,
        WARNING
    };

    float scrollOversizedTextPositionX;

    void resetTextScrollPosition()
    {
        scrollOversizedTextPositionX = 0.0;
    }

    void setTextSettings(TextState state, ImVec4 &textColor, ImVec4 &bgColor)
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

    void CenteredText(const std::string &label)
    {
        auto windowWidth = ImGui::GetWindowSize().x;
        auto windowHeight = ImGui::GetWindowSize().y;
        auto textWidth = ImGui::CalcTextSize(label.c_str()).x;
        auto textHeight = ImGui::CalcTextSize(label.c_str()).y;

        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::SetCursorPosY((windowHeight - textHeight) * 0.5f);
        ImGui::Text("%s", label.c_str());
    }

    void Text(const std::string &label, TextState textState)
    {
        ImVec4 textColor;
        ImVec4 bgColor;
        setTextSettings(textState, textColor, bgColor);

        int padding_x = 2;
        ImVec2 textSize = ImGui::CalcTextSize(label.c_str()); // Get text dimensions
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();       // Get screen position of the cursor

        int width = (int)ImGui::GetWindowSize().x;

        bool isOversized = textState == TextState::SELECTED && textSize.x >= width;
        if (isOversized)
        {
            // bgColor = ImVec4(255, 0, 0, 255);
            if (abs(scrollOversizedTextPositionX) < textSize.x - width + 10)
            {
                scrollOversizedTextPositionX -= 0.75;
            }
        }

        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(cursorPos.x - padding_x, cursorPos.y),
            ImVec2(cursorPos.x + textSize.x + padding_x * 2, cursorPos.y + textSize.y),
            ImGui::ColorConvertFloat4ToU32(bgColor) // Convert to ImU32
        );

        ImGui::PushStyleColor(ImGuiCol_Text, textColor);
        if (isOversized)
        {
            ImGui::SetCursorPosX(cursorPos.x + padding_x + scrollOversizedTextPositionX);
        }
        else
        {
            ImGui::SetCursorPosX(cursorPos.x + padding_x);
        }
        ImGui::Text("%s", label.c_str());
        ImGui::PopStyleColor();
    }

    void MediaButtonID(int id)
    {
        ImFont *font_big = FontManager::GetInstance().font_big;
        int width = (int)ImGui::GetWindowSize().x;
        int height = (int)ImGui::GetWindowSize().y;

        ImGui::PushFont(FontManager::GetInstance().font_big);

        ImDrawList *drawList = ImGui::GetForegroundDrawList();

        std::string text = std::to_string(id);
        ImVec2 textExtent = ImGui::CalcTextSize(text.c_str());

        // ImGui::PushClipRect({}, {1000, 1000}, false); // Disable clipping to prevent cutting corners

        int x = width - (int)textExtent.x;
        int y = 0;

        ImVec2 rectStart = ImVec2(x, y);
        ImVec2 rectEnd = ImVec2(
            x + static_cast<float>(textExtent.x),
            y + static_cast<float>(textExtent.y) - 4); // hard-coded 4 pixels less in height

        // printf("textExtend x: %f, y: %f // start-x: %d, start-y: %d, width: %d, height: %d\n", textExtent.x, textExtent.y, x, y, width, height);

        ImU32 rectColor = IM_COL32(255, 255, 255, 255);
        ImU32 textColor = IM_COL32(0, 0, 0, 255);
        drawList->AddRectFilled(rectStart, rectEnd, rectColor);
        // drawList->AddRectFilled(ImVec2(0, 0), ImVec2(width, textExtent.y - 4), rectColor);
        drawList->AddText(ImVec2(x, y), textColor, text.c_str());
        // drawList->AddRectFilled(ImVec2(0, rectEnd.y), ImVec2(width, rectEnd.y + 1), rectColor);
        drawList->AddRectFilled(ImVec2(rectStart.x - 3, 0), ImVec2(rectStart.x - 1, rectEnd.y), textColor);

        // ImGui::PopClipRect();
        ImGui::PopFont();
    }

    void MenuTitle(std::string menuTitle)
    {
        std::transform(menuTitle.begin(), menuTitle.end(), menuTitle.begin(), ::toupper);

        ImFont *font_big = FontManager::GetInstance().font_big;
        int width = (int)ImGui::GetWindowSize().x;
        int height = (int)ImGui::GetWindowSize().y;
        ImGui::PushFont(FontManager::GetInstance().font_big);

        ImDrawList *drawList = ImGui::GetForegroundDrawList();
        ImVec2 textExtent = ImGui::CalcTextSize(menuTitle.c_str());

        int x = 0;
        int y = 0;

        ImVec2 rectStart = ImVec2(x, y);
        ImVec2 rectEnd = ImVec2(
            x + static_cast<float>(textExtent.x),
            y + static_cast<float>(textExtent.y));

        ImU32 bgColor = IM_COL32(255, 255, 255, 255);
        ImU32 textColor = IM_COL32(0, 0, 0, 255);
        // drawList->AddRectFilled(rectStart, rectEnd, rectColor);
        drawList->AddRectFilled(ImVec2(0, 0), ImVec2(width, textExtent.y - 4), bgColor);
        drawList->AddText(ImVec2(x, y), textColor, menuTitle.c_str());
        ImGui::PopFont();
    }

    void InfoScreen(int bank, int id, std::string filename)
    {
        ImGui::SetCursorPosY(25);
        ImGui::Text("Information:");
        ImGui::Text("%s", filename.c_str());
        ImGui::Text("Currrent Pos/Duration");
        ImGui::Text("Loop yes or no");
        ImGui::Text("%d/%d", bank, id);
    }

    bool CheckBox(const std::string& label, bool selected, bool checked)
    {
        bool oldChecked= checked;
        bool keyPressed = (ImGui::IsKeyPressed(ImGuiKey_RightArrow) || (ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_DownArrow)));
        if (selected && keyPressed) {
            checked = !checked;
        }

        std::string newLabel = "[ ] " + label;
        if (checked) newLabel = "[x] " + label;
        Text(newLabel, selected ? TextState::SELECTED : TextState::DEFAULT);

        return checked != oldChecked;
    }

    bool RadioButton(const std::string& label, bool selected, bool toggled)
    {
        bool oldToggled = toggled;
        bool keyPressed = (ImGui::IsKeyPressed(ImGuiKey_RightArrow) || (ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_DownArrow)));
        if (selected && keyPressed) {
            toggled = !toggled;
        }

        std::string newLabel = "[ ] " + label;
        if (toggled) newLabel = "[*] " + label;
        Text(newLabel, selected ? TextState::SELECTED : TextState::DEFAULT);

        return selected && toggled;
    }


    // int getMenuNavInput(int* selectedIndex) {
    //     int leaveMenu = 0;
    //     if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
    //         *selectedIndex--;
    //     }
    //     else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
    //         *selectedIndex++;
    //     }
    //     else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
    //         *selectedIndex++;
    //         leaveMenu = -1;
    //     }

    //     return leaveMenu;
    // }

    // int FileSelection(const std::vector<std::string>& files, std::string* activeFile, int* selectedIndex)
    // {
    //     int leaveMenu = getMenuNavInput(selectedIndex);

    //     for (int i = 0; i < files.size(); ++i) {
    //         bool isSelected = (i == *selectedIndex);

    //         std::string file = files[i];
    //         if (Toggled(file, isSelected, (file == *activeFile))) {
    //             *activeFile = file;
    //         }
    //     }
    // }

    // int LiveInput(HdmiInputConfig* hdmiInputConfig, int* selectedIndex)
    // {
    //     if (!hdmiInputConfig) return;

    //     int leaveMenu = getMenuNavInput(selectedIndex);

    //     int i = 0;
    //     bool isHdmi1 = (hdmiInputConfig->hdmiPort == 0);
    //     if (Toggled("HDMI 1", (i++ == *selectedIndex), &isHdmi1)) {
    //         hdmiInputConfig->hdmiPort = 0;
    //     }
    //     bool isHdmi2 = (hdmiInputConfig->hdmiPort == 1);
    //     if (Toggled("HDMI 2", (i++ == *selectedIndex), &isHdmi2)) {
    //         hdmiInputConfig->hdmiPort = 1;
    //     }

    //     return leaveMenu;
    // }

    int FileSelection(Registry* registry, int id)
    {
        auto config = std::make_unique<VideoInputConfig>();
        VideoInputConfig* currentConfig = registry->inputMappings().getVideoInputConfig(id);
        if (currentConfig) {
            *config = *currentConfig;
        }

        // need to get that from registry
        std::vector<std::string>& files = registry->mediaPool().getVideoFiles();
        bool changed = false;
        for (int i = 0; i < files.size(); ++i) {
            std::string fileName = files[i];
            if (ImGui::RadioButton(fileName.c_str(), (currentConfig->fileName == fileName))) {
                currentConfig->fileName = fileName;
            }
        }

        if (changed)
            registry->inputMappings().addInputConfig(id, std::move(config));
    }

    void LiveInputSelection(Registry* registry, int id) {
        auto config = std::make_unique<HdmiInputConfig>();
        HdmiInputConfig* currentConfig = registry->inputMappings().getHdmiInputConfig(id);
        if (currentConfig) {
            *config = *currentConfig; 
        }

        bool changed = false;
        changed |= ImGui::RadioButton("HDMI 1", &(config->hdmiPort), 0);
        changed |= ImGui::RadioButton("HDMI 2", &(config->hdmiPort), 1);

        if (changed)
            registry->inputMappings().addInputConfig(id, std::move(config));
    }

    void HierarchicalMenuWidget(const MenuItem& root, Registry* registry) {
        static std::vector<int> path;
        static int selectedIdx = 0;

        // Traverse to current menu
        const MenuItem* current = &root;
        for (int idx : path) {
            if (idx >= 0 && idx < (int)current->children.size())
                current = &current->children[idx];
            else
                break;
        }

        // Render current menu page
        ImGui::BeginChild("MenuPage", ImVec2(0, 200), true);
        for (int i = 0; i < (int)current->children.size(); ++i) {
            bool isSelected = (i == selectedIdx);
            std::string label = current->children[i].label;
            if (ImGui::Selectable(label.c_str(), isSelected)) {
                selectedIdx = i;
            }
        }
        ImGui::EndChild();

        // Render dynamic content if present
        if (selectedIdx >= 0 && selectedIdx < (int)current->children.size()) {
            auto& selected = current->children[selectedIdx];
            if (selected.renderFunc) {
                ImGui::Separator();
                selected.renderFunc(registry, 0);
            }
        }

        // Handle navigation
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
                // Go deeper if selected item has children or a render_func (treat as a page)
                if (selectedIdx >= 0 && selectedIdx < (int)current->children.size()) {
                    auto& sel = current->children[selectedIdx];
                    if (!sel.children.empty() || sel.renderFunc) {
                        path.push_back(selectedIdx);
                        selectedIdx = 0;
                    }
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
                // Go up one level
                if (!path.empty()) {
                    path.pop_back();
                    selectedIdx = 0;
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
                if (selectedIdx > 0) selectedIdx--;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
                if (selectedIdx + 1 < (int)current->children.size()) selectedIdx++;
            }
        }
    }
}