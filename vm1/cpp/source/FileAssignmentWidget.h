/*
 * Copyright (c) 2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include <imgui.h>
#include <filesystem>
#include <vector>
#include <string>


//#include "VideoPlane.h"
#include "PlaybackOperator.h"
#include "Registry.h"

class FileAssignmentWidget
{
private:
    static const int SPACING = 6;
    static const int WIDTH = 8;
    static const int HEIGHT = 2;

    PlaybackOperator& m_playbackOperator;
    Registry& m_registry;
    //VideoPlane *m_videoPlaneLeft = nullptr;
    //VideoPlane *m_videoPlaneRight = nullptr;

    //std::string m_directory;
    //std::vector<std::string> m_files;
    std::string m_selectedFile;
    //std::string m_assignedFiles[HEIGHT][WIDTH];
    int m_bank = 0;

    ImVec4 m_defaultButtonColor = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    ImVec4 m_assignedButtonColor = ImVec4(0.0f, 0.5f, 0.0f, 1.0f);
    ImVec4 m_highlightedButtonColor = ImVec4(0.5f, 0.5f, 0.0f, 1.0f);

    // std::string m_keyLabels[HEIGHT][WIDTH] = {  {"Q", "W", "E", "R", "T", "Z", "U", "I"},
    //                                             {"A", "S", "D", "F", "G", "H", "J", "K"} };
    std::string m_keyLabels[HEIGHT][WIDTH] = {{"A", "S", "D", "F", "G", "H", "J", "K"},
                                              {"Z", "X", "C", "V", "B", "N", "M", ","}};

    // ImGuiKey m_keyboardShortcuts[HEIGHT][WIDTH] = {{ImGuiKey_Q, ImGuiKey_W, ImGuiKey_E, ImGuiKey_R, ImGuiKey_T, ImGuiKey_Z, ImGuiKey_U, ImGuiKey_I},
    //                                                {ImGuiKey_A, ImGuiKey_S, ImGuiKey_D, ImGuiKey_F, ImGuiKey_G, ImGuiKey_H, ImGuiKey_J, ImGuiKey_K}};
    ImGuiKey m_keyboardShortcuts[HEIGHT][WIDTH] = {{ImGuiKey_A, ImGuiKey_S, ImGuiKey_D, ImGuiKey_F, ImGuiKey_G, ImGuiKey_H, ImGuiKey_J, ImGuiKey_K},
                                                   {ImGuiKey_Z, ImGuiKey_X, ImGuiKey_C, ImGuiKey_V, ImGuiKey_B, ImGuiKey_N, ImGuiKey_M, ImGuiKey_Comma}};

public:
    FileAssignmentWidget(PlaybackOperator& playbackOperator, Registry &registry);

    void render();

private:
    void setupKeyboardShortcuts();
    void setupKeyLabels();
    //void loadFiles(const std::string &directory);
    void renderFileList();
    void renderButtonMatrix();
    void renderSettings();
    //void handleButtonClick(int id);
    void handleKeyboardShortcuts();
    bool isButtonHighlighted(int x, int y);
};