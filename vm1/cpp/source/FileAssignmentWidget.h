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

#include "VideoPlane.h"

class FileAssignmentWidget {
private:
    static const int SPACING = 6;
    static const int WIDTH = 8;
    static const int HEIGHT = 2;
    
    VideoPlane* m_videoPlaneLeft = nullptr;
    VideoPlane* m_videoPlaneRight = nullptr;
    std::string m_directory;
    std::vector<std::string> m_files;
    std::string m_selectedFile;
    std::string m_assignedFiles[HEIGHT][WIDTH];
    
    ImVec4 m_defaultButtonColor = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    ImVec4 m_assignedButtonColor = ImVec4(0.0f, 0.5f, 0.0f, 1.0f);
    ImVec4 m_highlightedButtonColor = ImVec4(0.5f, 0.5f, 0.0f, 1.0f);

    std::string m_keyLabels[HEIGHT][WIDTH] = {  {"Q", "W", "E", "R", "T", "Z", "U", "I"}, 
                                                {"A", "S", "D", "F", "G", "H", "J", "K"} };

    ImGuiKey m_keyboardShortcuts[HEIGHT][WIDTH] = { {ImGuiKey_Q, ImGuiKey_W, ImGuiKey_E, ImGuiKey_R, ImGuiKey_T, ImGuiKey_Z, ImGuiKey_U, ImGuiKey_I}, 
                                                    {ImGuiKey_A, ImGuiKey_S, ImGuiKey_D, ImGuiKey_F, ImGuiKey_G, ImGuiKey_H, ImGuiKey_J, ImGuiKey_K} };

public:
    FileAssignmentWidget(const std::string& directory, VideoPlane* videoPlaneLeft, VideoPlane* videoPlaneRight);
    
    void render();

private:
    void setupKeyboardShortcuts();
    void setupKeyLabels();
    void loadFiles(const std::string& directory);
    void renderFileList();
    void renderButtonMatrix();
    void handleButtonClick(int row, int col);
    void handleKeyboardShortcuts();
    bool isButtonHighlighted(int x, int y);
};