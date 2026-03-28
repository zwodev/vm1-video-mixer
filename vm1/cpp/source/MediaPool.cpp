#include <iostream>
#include <filesystem>
#include <functional>

#include "MediaPool.h"

MediaPool::MediaPool()
{
    loadQrCodeImageBuffer();
    startDirectoryWatcher();
}

std::vector<std::string>& MediaPool::getVideoFiles()
{
    return m_videoFiles;
}

std::string MediaPool::getVideoFilePath(const std::string& fileName)
{
    return m_videoFilePath + fileName;
}

std::vector<std::string> MediaPool::GetMediaFilesPendingPreview()
{
    return m_mediaFilesPendingPreview; 
}

void MediaPool::updateVideoFiles()
{
    m_videoFiles.clear();
    for (const auto &entry : std::filesystem::recursive_directory_iterator(m_videoFilePath))
    {

        if (entry.is_regular_file() && entry.path().extension() != ".preview")
        {
            std::string filename = entry.path().filename().string();
            std::string filePath = filename;
            m_videoFiles.push_back(filePath);
        }
    }

    // sort the files by name
    std::sort(m_videoFiles.begin(), m_videoFiles.end());
}

void MediaPool::updateVideoPreviewFiles()
{
    m_videoFilesPreviews.clear();
    for (const auto &entry : std::filesystem::recursive_directory_iterator(m_videoFilePath))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".preview")
        {
            std::string filename = entry.path().filename().string();
            std::string filePath = filename;
            m_videoFilesPreviews.push_back(filePath);
        }
    }
}

void MediaPool::updateVideoFilesPendingPreviews()
{
    m_mediaFilesPendingPreview.clear();
    // compare m_videoFilesPreviews with m_videoFiles
    for(const auto& videoFile : m_videoFiles) 
    {
        std::string previewFileName = videoFile + ".preview";
        if(std::find(m_videoFilesPreviews.begin(), m_videoFilesPreviews.end(), previewFileName) == m_videoFilesPreviews.end()) 
        {
            m_mediaFilesPendingPreview.push_back(m_videoFilePath + videoFile);
        }
    }
}

std::vector<std::string>& MediaPool::getGenerativeShaderFiles()
{
    // TODO: Update only when new files are present.
    updateGenerativeShaderFiles();
    return m_generativeShaderFiles;
}

std::string MediaPool::getGenerativeShaderFilePath(const std::string& fileName)
{
    return m_generativeShaderPath + fileName;
}

void MediaPool::updateGenerativeShaderFiles()
{
    m_generativeShaderFiles.clear();
    for (const auto &entry : std::filesystem::directory_iterator(m_generativeShaderPath))
    {
        if (entry.is_regular_file())
        {
            std::string filename = entry.path().filename().string();
            std::string filePath = filename;
            m_generativeShaderFiles.push_back(filePath);
        }
    }

    // sort the files by name
    std::sort(m_generativeShaderFiles.begin(), m_generativeShaderFiles.end());
}

std::string MediaPool::getEffectShaderFilePath(const std::string& fileName)
{
    return m_effectShaderPath + fileName;
}

std::vector<std::string>& MediaPool::getEffectShaderFiles()
{
    // TODO: Update only when new files are present.
    updateEffectShaderFiles();
    return m_effectShaderFiles;
}

void MediaPool::updateEffectShaderFiles()
{
    m_effectShaderFiles.clear();
    for (const auto &entry : std::filesystem::directory_iterator(m_effectShaderPath))
    {
        if (entry.is_regular_file())
        {
            std::string filename = entry.path().filename().string();
            std::string filePath = filename;
            m_effectShaderFiles.push_back(filePath);
        }
    }

    // sort the files by name
    std::sort(m_effectShaderFiles.begin(), m_effectShaderFiles.end());
}

void MediaPool::loadQrCodeImageBuffer()
{
    int width, height, channels;
    std::string filename("data/wifi_qr.png");
    unsigned char *data = stbi_load(
        filename.c_str(),
        &width,
        &height,
        &channels,
        0);

    if (data == nullptr)
    {
        std::cerr << "Failed to load PNG: " << filename << std::endl;
        return;
    }

    std::cout << "Loaded PNG: " << filename << std::endl;
    std::cout << "Size: " << width << " x " << height << std::endl;
    std::cout << "Channels: " << channels << std::endl;

    // if (channels != 3)
    //     return;

    m_qrCodeImageBuffer = ImageBuffer(width, height, channels, reinterpret_cast<char*>(data));
}

const ImageBuffer& MediaPool::getQrCodeImageBuffer()
{
    return m_qrCodeImageBuffer;
}

void MediaPool::runMediaDirectoryWatcher()
{
    while(m_isWatcherRunning)
    {
        updateVideoFiles();
        updateVideoPreviewFiles();
        updateVideoFilesPendingPreviews();
        for(const auto& file : m_mediaFilesPendingPreview){
            generateVideoFilePreview(file);
        }
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void MediaPool::generateVideoFilePreview(std::string filename)
{
    printf("Generating preview png for: %s\n", filename.c_str());
    // ffmpeg -i oberbaum-01.mov -vf "select='not(mod(n\,12))',scale=160:-1,tile=1x5" -frames:v 1 oberbaum-01-2.png
    
    char execCommand[512];
    sprintf(execCommand, "ffmpeg -i %s -vf \"scale=160:-1,tile=10x10\" -frames:v 1 %s.png >> ffmpeg_log.txt 2>&1", filename.c_str(), filename.c_str());
    printf("EXEC: %s\n", execCommand);
    
    if(std::system(execCommand) == 0)
    {
        printf("ffmpeg successful\n");
        sprintf(execCommand, "mv %s.png %s.preview", filename.c_str(), filename.c_str());
        printf("EXEC: %s\n", execCommand);
        std::system(execCommand);
    }
}

void MediaPool::startDirectoryWatcher() 
{
    stopDirectoryWatcher();
    m_isWatcherRunning = true;
    m_thread = std::thread(&MediaPool::runMediaDirectoryWatcher, this);

}

void MediaPool::stopDirectoryWatcher() 
{
    m_isWatcherRunning = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}