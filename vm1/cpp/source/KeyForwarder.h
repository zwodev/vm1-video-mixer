#pragma once

#include <iostream>
#include <array>
#include <imgui.h>

class KeyForwarder {
private:
    std::array<bool, 4> previousKeyStates;
    std::array<ImGuiKey, 4> arrowKeys = {
        ImGuiKey_LeftArrow, ImGuiKey_RightArrow,
        ImGuiKey_UpArrow, ImGuiKey_DownArrow
    };

public:
    KeyForwarder() : previousKeyStates{false, false, false, false} {}

    void ForwardArrowKeys(ImGuiContext* sourceContext, ImGuiContext* targetContext) {
        ImGui::SetCurrentContext(sourceContext);
        
        for (size_t i = 0; i < arrowKeys.size(); ++i) {
            bool currentKeyState = ImGui::IsKeyDown(arrowKeys[i]);
            
            if (currentKeyState != previousKeyStates[i]) {
                ImGui::SetCurrentContext(targetContext);
                if (currentKeyState) {
                    ImGui::GetIO().AddKeyEvent(arrowKeys[i], true);
                    ImGui::GetIO().AddKeyEvent(arrowKeys[i], false);  // Immediate release
                }
                ImGui::SetCurrentContext(sourceContext);
                
                previousKeyStates[i] = currentKeyState;
                
                // Debug output
                printf("Key %d state changed to: %s\n", arrowKeys[i], currentKeyState ? "Pressed" : "Released");
            }
        }
    }
};
