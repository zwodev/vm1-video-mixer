#pragma once

#include "imgui.h"
#include "FontManager.h"
#include <string>
#include <algorithm>

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

    float scrollOversizedTextPositionX;

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

    void resetTextScrollPosition()
    {
        scrollOversizedTextPositionX = 0.0;
    }

    void renderText(const std::string &label, TextState textState)
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

    void renderMediaButtonID(int id)
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

    void renderMenuTitle(std::string menuTitle)
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

    void renderInfoScreen(int m_bank, int m_id, std::string filename)
    {
        ImGui::SetCursorPosY(25);
        ImGui::Text("Information:");
        ImGui::Text("%s", filename.c_str());
        ImGui::Text("Currrent Pos/Duration");
        ImGui::Text("Loop yes or no");
        ImGui::Text("%d/%d", m_bank, m_id);
    }

}