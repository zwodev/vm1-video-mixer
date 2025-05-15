/*
 * Copyright (c) 2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include "FileAssignmentWidget.h"

FileAssignmentWidget::FileAssignmentWidget(PlaybackOperator& playbackOperator, Registry& registry) :
    m_playbackOperator(playbackOperator),
    m_registry(registry)
{
}


void FileAssignmentWidget::renderFileList() {
    
    ImGui::BeginChild("FileList", ImVec2(0, 0), true);
    for (const auto& file : m_registry.mediaPool().getVideoFiles()) {
        if (ImGui::Selectable(file.c_str(), m_selectedFile == file)) {
            m_selectedFile = file;
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("FILE_PAYLOAD", file.c_str(), file.length() + 1);
            ImGui::Text("Dragging: %s", file.c_str());
            ImGui::EndDragDropSource();
        }
    }
    ImGui::EndChild();
}

void FileAssignmentWidget::renderButtonMatrix() {
    ImGui::BeginChild("Button Matrix");
    float buttonSize = (ImGui::GetContentRegionAvail().x - ((WIDTH + 1) * SPACING)) / WIDTH;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            ImGui::PushID(y * WIDTH + x);
            
            //ImVec4 buttonColor = m_assignedFiles[y][x].empty() ? m_defaultButtonColor : m_assignedButtonColor;
            int id = m_bank * (WIDTH * HEIGHT) + y * WIDTH + x;
            std::string filePath;
            VideoInputConfig* videoInputConfig = m_registry.inputMappings().getVideoInputConfig(id);
            if (videoInputConfig) {
                filePath = videoInputConfig->fileName;
            }
            
            ImVec4 buttonColor = filePath.empty() ? m_defaultButtonColor : m_assignedButtonColor;
            //if (isButtonHighlighted(x, y)) {
            //    buttonColor = m_highlightedButtonColor;
            //}
            
            ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);

            std::string buttonLabel = m_keyLabels[y][x];
            
            if (ImGui::Button(buttonLabel.c_str(), ImVec2(buttonSize, buttonSize))) {
                //handleButtonClick(id);
                m_playbackOperator.showMedia(id);
            }

            ImGui::PopStyleVar();
            ImGui::PopStyleColor();

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PAYLOAD")) {
                    auto videoInputConfig = std::make_unique<VideoInputConfig>();
                    videoInputConfig->fileName = m_registry.mediaPool().getVideoFilePath(std::string(static_cast<const char*>(payload->Data)));
                    m_registry.inputMappings().addInputConfig(id, std::move(videoInputConfig));
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PopID();
            ImGui::SameLine();
        }
        ImGui::NewLine();
    }
    ImGui::EndChild();
}

void FileAssignmentWidget::renderSettings() {
    ImGui::BeginChild("Settings", ImVec2(0, 0), true);
    ImGui::SliderInt("Bank", &m_bank, 0, 3);
    ImGui::EndChild();
}

void FileAssignmentWidget::render() {
    Settings& settings = m_registry.settings();
    
    if (settings.showUI) {
        ImGui::Begin("File Assignment");

        ImGui::Columns(3);
        // Left column: File list
        renderFileList();
        ImGui::NextColumn();
        // Middle column: Button matrix
        renderButtonMatrix();
        ImGui::NextColumn();
        // Right column: Render settings
        renderSettings();

        ImGui::End();
    }

    handleKeyboardShortcuts();
}

// void FileAssignmentWidget::handleButtonClick(int id) {
//     std::string fileName;
//     std::string filePath;
//     bool looping = false;

//     InputConfig* inputConfig = m_registry.inputMappings().getInputConfig(id);
//     if (VideoInputConfig* videoInputConfig = dynamic_cast<VideoInputConfig*>(inputConfig)) {
//         fileName = videoInputConfig->fileName;
//         looping = videoInputConfig->looping;
//         filePath = m_registry.mediaPool().getVideoFilePath(fileName);
//     }
//     else if (HdmiInputConfig* hdmiInputConfig = dynamic_cast<HdmiInputConfig*>(inputConfig)) {
//         if (hdmiInputConfig->hdmiPort == 0) {
//             fileName = "hdmi0";
//             filePath = m_registry.mediaPool().getVideoFilePath(fileName);
//         }
//     }
//     else {
//         return;
//     }

//     int oddRow = (id / WIDTH) % 2;
//     // Select plane
//     if (oddRow == 0) {
//         printf("Play Left: (ID: %d, FILE: %s, LOOP: %d)\n", id, fileName.c_str(), looping);
//         m_videoPlaneLeft->playAndFade(filePath, looping);
//     } 
//     else {
//         printf("Play Right: (ID: %d, FILE: %s, LOOP: %d)\n", id, fileName.c_str(), looping);
//         m_videoPlaneRight->playAndFade(filePath, looping);
//     }
// }

void FileAssignmentWidget::handleKeyboardShortcuts() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            ImGuiKey key = m_keyboardShortcuts[y][x];
            if (ImGui::IsKeyPressed(key) && !ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
                int id = m_bank * (2 * WIDTH) + (y * WIDTH) + x;
                //handleButtonClick(id);
                m_playbackOperator.showMedia(id);
                return;
            }
        }
    }
}

bool FileAssignmentWidget::isButtonHighlighted(int x, int y) {
    ImGuiKey key = m_keyboardShortcuts[y][x];
    return ImGui::IsKeyDown(key);
}
