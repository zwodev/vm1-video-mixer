/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include <iostream>
#include <filesystem>
#include <functional>

#include "MediaPool.h"

MediaPool::MediaPool()
{
    loadQrCodeImageBuffer();
    startDirectoryWatcher();
}

MediaPool::~MediaPool()
{
    stopDirectoryWatcher();
}

const ImageBuffer& MediaPool::getLogo()
{
    return m_logo;
}

std::vector<DirectoryEntry> MediaPool::getVideoDirectoryEntries(const std::string& path)
{
    return m_directoryCache.getEntries(getVideoFilePath(path));
}

std::string MediaPool::getVideoFilePath(const std::string& fileName)
{
    return m_videoFilePath + fileName;
}

void MediaPool::updateVideoFilePreviews()
{
    std::vector<std::string> files;
    std::vector<std::string> previewFiles;

    for (auto it = std::filesystem::recursive_directory_iterator(m_videoFilePath);
        it != std::filesystem::recursive_directory_iterator{}; ++it)
    {
        auto &entry = *it;
        if (entry.is_directory()) {
            auto name = entry.path().filename().string();
            if (!name.empty() && name[0] == '.') {
                it.disable_recursion_pending();
                continue;
            }
        }
        if (entry.is_regular_file()) {
            std::string filePath = entry.path().string();
            if (entry.path().extension() == ".preview")
            {
                previewFiles.push_back(filePath);
            }
            else
            {
                files.push_back(filePath);
            }
        }
    }

    // compare m_videoFilesPreviews with m_videoFiles
    std::vector<std::string> pendingPreviewFiles;
    for(const auto& videoFile : files) 
    {
        std::string previewFileName = videoFile + ".preview";
        if(std::find(previewFiles.begin(), previewFiles.end(), previewFileName) == previewFiles.end()) 
        {
            pendingPreviewFiles.push_back(videoFile);
        }
    }

    // finally create missing previews
    for(const auto& file : pendingPreviewFiles){
        generateVideoFilePreview(file);
    }
}

std::string MediaPool::getGenerativeShaderFilePath(const std::string& fileName)
{
    return m_generativeShaderPath + fileName;
}

std::vector<DirectoryEntry> MediaPool::getGenerativeShaderFiles(const std::string& path)
{
    return m_directoryCache.getEntries(getGenerativeShaderFilePath(path));
}

std::string MediaPool::getEffectShaderFilePath(const std::string& fileName)
{
    return m_effectShaderPath + fileName;
}

std::vector<DirectoryEntry> MediaPool::getEffectShaderFiles(const std::string& path)
{
    return m_directoryCache.getEntries(getEffectShaderFilePath(path));
}

std::shared_ptr<PreviewNode> MediaPool::getPreview(const std::string& path)
{
    return m_previewCache.getEntry(getVideoFilePath(path));
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
        updateVideoFilePreviews();
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
        // sprintf(execCommand, "rm -rf %s.hist", m_videoFilePath.c_str());
        // printf("EXEC: %s\n", execCommand);
        // std::system(execCommand);
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