#pragma once

#include <iostream>
#include <array>
#include <imgui.h>

class KeyForwarder
{
public:
    KeyForwarder()
    {
        for (auto key : m_arrowKeys)
        {
            m_previousKeyStates.push_back(false);
        }
    }

    void forwardArrowKeys(ImGuiContext *sourceContext, ImGuiContext *targetContext)
    {
        ImGui::SetCurrentContext(sourceContext);
        for (size_t i = 0; i < m_arrowKeys.size(); ++i)
        {
            bool currentKeyState = ImGui::IsKeyDown(m_arrowKeys[i]);
            if (currentKeyState != m_previousKeyStates[i])
            {
                ImGui::SetCurrentContext(targetContext);
                if (currentKeyState)
                {
                    ImGui::GetIO().AddKeyEvent(m_arrowKeys[i], true);
                }
                else
                {
                    ImGui::GetIO().AddKeyEvent(m_arrowKeys[i], false);
                }
                ImGui::SetCurrentContext(sourceContext);
                m_previousKeyStates[i] = currentKeyState;
            }
        }
    }

private:
    std::vector<bool> m_previousKeyStates;
    std::vector<ImGuiKey> m_arrowKeys = {ImGuiKey_LeftShift, ImGuiKey_LeftArrow, ImGuiKey_RightArrow, ImGuiKey_UpArrow, ImGuiKey_DownArrow,
                                         //  ImGuiKey_Q, ImGuiKey_W, ImGuiKey_E, ImGuiKey_R,
                                         //  ImGuiKey_T, ImGuiKey_Y, ImGuiKey_U, ImGuiKey_I,
                                         ImGuiKey_A, ImGuiKey_S, ImGuiKey_D, ImGuiKey_F,
                                         ImGuiKey_G, ImGuiKey_H, ImGuiKey_J, ImGuiKey_K,
                                         ImGuiKey_Z, ImGuiKey_X, ImGuiKey_C, ImGuiKey_V,
                                         ImGuiKey_B, ImGuiKey_N, ImGuiKey_M, ImGuiKey_Comma

    };
};
