#include "FileAssignmentWidget.h"

FileAssignmentWidget::FileAssignmentWidget(const std::string& directory, VideoPlane* videoPlaneLeft, VideoPlane* videoPlaneRight) {
    m_videoPlaneLeft = videoPlaneLeft;
    m_videoPlaneRight = videoPlaneRight;
    m_directory = directory;
    loadFiles(directory);


    // for (int y = 0; y < HEIGHT; y++) {
    //     int startKey = (int)ImGuiKey_Q;
    //     if (y > 0) startKey = (int)ImGuiKey_A;
    //     for (int x = 0; x < WIDTH; x++) {
    //         ImGuiKey key = (ImGuiKey)(startKey + x);
    //         m_keyboardShortcuts[x][y] = key;
    //     }
    // }
}

// void FileAssignmentWidget::renderFileList() {
//     ImGui::BeginChild("FileList", ImVec2(0, 0), true);
//     for (const auto& file : files) {
//         if (ImGui::Selectable(file.c_str(), selectedFile == file)) {
//             selectedFile = file;
//         }

//         if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
//             ImGui::SetDragDropPayload("FILE_PAYLOAD", file.c_str(), file.length() + 1);
//             ImGui::Text("Dragging: %s", file.c_str());
//             ImGui::EndDragDropSource();
//         }
//     }
//     ImGui::EndChild();
// }

// void FileAssignmentWidget::renderButtonMatrix() {
//     float buttonSize = ImGui::GetContentRegionAvail().x / WIDTH;
//     for (int y = 0; y < HEIGHT; y++) {
//         for (int x = 0; x < WIDTH; x++) {
//             ImGui::PushID(y * WIDTH + x);
            
//             ImVec4 buttonColor = assignedFiles[x][y].empty() ? defaultButtonColor : assignedButtonColor;
//             if (isButtonHighlighted(x, y)) {
//                 buttonColor = highlightedButtonColor;
//             }
            
//             ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
//             ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);

//             std::string buttonLabel = std::to_string(i + 1) + std::string(1, 'Q' + j);
//             if (ImGui::Button(buttonLabel.c_str(), ImVec2(buttonSize, buttonSize))) {
//                 handleButtonClick(i, j);
//             }

//             ImGui::PopStyleVar();
//             ImGui::PopStyleColor();

//             if (ImGui::BeginDragDropTarget()) {
//                 if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PAYLOAD")) {
//                     assignedFiles[i][j] = static_cast<const char*>(payload->Data);
//                 }
//                 ImGui::EndDragDropTarget();
//             }

//             ImGui::PopID();
//             ImGui::SameLine();
//         }
//         ImGui::NewLine();
//     }
// }

void FileAssignmentWidget::render() {
    ImGui::Begin("File Assignment");

    ImGui::Columns(2);

    // Left column: File list
    ImGui::BeginChild("FileList", ImVec2(0, 0), true);
    for (const auto& file : files) {
        if (ImGui::Selectable(file.c_str(), selectedFile == file)) {
            selectedFile = file;
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("FILE_PAYLOAD", file.c_str(), file.length() + 1);
            ImGui::Text("Dragging: %s", file.c_str());
            ImGui::EndDragDropSource();
        }
    }
    ImGui::EndChild();

    ImGui::NextColumn();

    // Right column: Button matrix
    const int WIDTH = 8;
    const int HEIGHT = 2;
    const int SPACING = 6;
    float buttonSize = (ImGui::GetContentRegionAvail().x - ((WIDTH + 1) * SPACING)) / WIDTH;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int id = y * WIDTH + x;
            //printf("Button Created: %d\n", id);
            ImGui::PushID(id);
            ImGui::PushStyleColor(ImGuiCol_Button, assignedFiles[x][y].empty() ? defaultButtonColor : assignedButtonColor);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);

            if (ImGui::Button("##", ImVec2(buttonSize, buttonSize))) {
                handleButtonClick(x, y);
            }

            
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PAYLOAD")) {
                    assignedFiles[x][y] = static_cast<const char*>(payload->Data);
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PopID();
            ImGui::SameLine();
        }
        ImGui::NewLine();
    }

    ImGui::End();
}

void FileAssignmentWidget::loadFiles(const std::string& directory) {
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path().filename().string());
        }
    }
}

void FileAssignmentWidget::handleButtonClick(int x, int y) {
    printf("Play: (%d, %d)\n", x, y);
    // This function will be called when a button is clicked
    // You can implement custom behavior here
    if (!assignedFiles[x][y].empty()) {
        // Do something with the assigned file
        std::string fileName = assignedFiles[x][y];
        
        // Add your custom logic here
        if (y == 0) {
            m_videoPlaneLeft->players()[0]->close();
            m_videoPlaneLeft->players()[0]->open(m_directory + "/" + fileName);
            m_videoPlaneLeft->players()[0]->play();
            
        } 
        else {
            m_videoPlaneRight->players()[0]->close();
            m_videoPlaneRight->players()[0]->open(m_directory + "/" + fileName);
            m_videoPlaneRight->players()[0]->play();
        }
    }
}

// void FileAssignmentWidget::handleKeyboardShortcuts() {
//     for (int y = 0; y < HEIGHT; y++) {
//         if (ImGui::IsKeyPressed(ImGuiKey_1 + i)) {
//             for (int x = 0; x < WIDTH; x++) {
//                 if (ImGui::IsKeyPressed(ImGuiKey_Q + j)) {
//                     handleButtonClick(x, y);
//                     return;
//                 }
//             }
//         }
//     }
// }

// bool FileAssignmentWidget::isButtonHighlighted(int x, int y) {
//     return ImGui::IsKeyDown(ImGuiKey_1 + row) || ImGui::IsKeyDown(ImGuiKey_Q + col);
// }
