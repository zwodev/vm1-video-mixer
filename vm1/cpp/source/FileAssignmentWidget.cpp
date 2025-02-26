#include "FileAssignmentWidget.h"

FileAssignmentWidget::FileAssignmentWidget(const std::string& directory, VideoPlane* videoPlaneLeft, VideoPlane* videoPlaneRight) {
    m_videoPlaneLeft = videoPlaneLeft;
    m_videoPlaneRight = videoPlaneRight;
    m_directory = directory;
    loadFiles(directory);
}


void FileAssignmentWidget::renderFileList() {
    ImGui::BeginChild("FileList", ImVec2(0, 0), true);
    for (const auto& file : m_files) {
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
    float buttonSize = (ImGui::GetContentRegionAvail().x - ((WIDTH + 1) * SPACING)) / WIDTH;
    //float buttonSize = ImGui::GetContentRegionAvail().x / WIDTH;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            ImGui::PushID(y * WIDTH + x);
            
            ImVec4 buttonColor = m_assignedFiles[y][x].empty() ? m_defaultButtonColor : m_assignedButtonColor;
            if (isButtonHighlighted(x, y)) {
                buttonColor = m_highlightedButtonColor;
            }
            
            ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);

            std::string buttonLabel = m_keyLabels[y][x];
            
            if (ImGui::Button(buttonLabel.c_str(), ImVec2(buttonSize, buttonSize))) {
                handleButtonClick(x, y);
            }

            ImGui::PopStyleVar();
            ImGui::PopStyleColor();

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PAYLOAD")) {
                    m_assignedFiles[y][x] = static_cast<const char*>(payload->Data);
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PopID();
            ImGui::SameLine();
        }
        ImGui::NewLine();
    }
}

void FileAssignmentWidget::render() {
    ImGui::Begin("File Assignment");

    ImGui::Columns(2);
    // Left column: File list
    renderFileList();
    ImGui::NextColumn();
    // Right column: Button matrix
    renderButtonMatrix();
    ImGui::End();

    handleKeyboardShortcuts();
}

void FileAssignmentWidget::loadFiles(const std::string& directory) {
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            m_files.push_back(entry.path().filename().string());
        }
    }
}

void FileAssignmentWidget::handleButtonClick(int x, int y) {
    if (!m_assignedFiles[y][x].empty()) {
        std::string fileName = m_assignedFiles[y][x];
        
        printf("Play: (%d, %d)\n", x, y);

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

void FileAssignmentWidget::handleKeyboardShortcuts() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            ImGuiKey key = m_keyboardShortcuts[y][x];
            if (ImGui::IsKeyPressed(key)) {
                handleButtonClick(x, y);
                return;
            }
        }
    }
}

bool FileAssignmentWidget::isButtonHighlighted(int x, int y) {
    ImGuiKey key = m_keyboardShortcuts[y][x];
    return ImGui::IsKeyDown(key);
}
