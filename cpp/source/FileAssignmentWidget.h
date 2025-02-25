#include <imgui.h>
#include <filesystem>
#include <vector>
#include <string>

#include "VideoPlane.h"

class FileAssignmentWidget {
private:
    static const int WIDTH = 8;
    static const int HEIGHT = 2;
    VideoPlane* m_videoPlaneLeft = nullptr;
    VideoPlane* m_videoPlaneRight = nullptr;
    std::string m_directory;
    std::vector<std::string> files;
    std::string selectedFile;
    std::string assignedFiles[WIDTH][HEIGHT];
    ImGuiKey m_keyboardShortcuts[WIDTH][HEIGHT];
    ImVec4 defaultButtonColor = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    ImVec4 assignedButtonColor = ImVec4(0.0f, 0.5f, 0.0f, 1.0f);

public:
    FileAssignmentWidget(const std::string& directory, VideoPlane* videoPlaneLeft, VideoPlane* videoPlaneRight);
    void render();

private:
    void loadFiles(const std::string& directory);
    void renderFileList();
    void renderButtonMatrix();
    void handleButtonClick(int row, int col);
};
