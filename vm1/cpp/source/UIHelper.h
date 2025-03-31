#pragma once

#include "imgui.h"
#include <string>

namespace UI
{
    enum TextState
    {
        DEFAULT,
        HIGHLIGHT,
        SELECTED,
        ERROR,
        WARNING
    };

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

    void renderCenteredText(const std::string &label)
    {
        auto windowWidth = ImGui::GetWindowSize().x;
        auto windowHeight = ImGui::GetWindowSize().y;
        auto textWidth = ImGui::CalcTextSize(label.c_str()).x;
        auto textHeight = ImGui::CalcTextSize(label.c_str()).y;

        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::SetCursorPosY((windowHeight - textHeight) * 0.5f);
        ImGui::Text("%s", label.c_str());
    }

    void renderText(const std::string &label, TextState textState)
    {

        ImVec4 textColor;
        ImVec4 bgColor;
        setTextSettings(textState, textColor, bgColor);

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

    void renderOverlayText(const std::string &text)
    {
        ImDrawList *drawList = ImGui::GetForegroundDrawList();
        drawList->AddText(ImVec2(100, 10), IM_COL32(0, 255, 0, 255), text.c_str());
    }
}