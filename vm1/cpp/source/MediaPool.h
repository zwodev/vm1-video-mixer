/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include <vector>
#include <string>
#include <thread>
#include <mutex>

#include "ImageBuffer.h"
#include "DirectoryCache.h"
#include "PreviewCache.h"

class MediaPool
{
public:
    MediaPool();
    ~MediaPool();

    const ImageBuffer& getLogo();

    std::string getVideoFilePath(const std::string& fileName);
    std::string getVideoFilesPath();
    std::vector<DirectoryEntry> getVideoDirectoryEntries(const std::string& path = "");

    std::string getGenerativeShaderFilePath(const std::string& fileName);
    std::vector<DirectoryEntry> getGenerativeShaderFiles(const std::string& path = "");
    
    std::string getEffectShaderFilePath(const std::string& fileName);
    std::vector<DirectoryEntry> getEffectShaderFiles(const std::string& path = "");

    std::shared_ptr<PreviewNode> getPreview(const std::string& path);

    void loadQrCodeImageBuffer();
    const ImageBuffer& getQrCodeImageBuffer();

private:
    void runMediaDirectoryWatcher();
    void startDirectoryWatcher();
    void stopDirectoryWatcher();
    void updateVideoFilePreviews();
    void generateVideoFilePreview(std::string filename);

private:
    ImageBuffer m_logo = ImageBuffer("media/splash-screen.png");
    std::string m_videoFilePath = "../videos/";
    std::string m_generativeShaderPath = "../shaders/generative/";
    std::string m_effectShaderPath = "../shaders/effect/";

    DirectoryCache m_directoryCache;
    PreviewCache m_previewCache;

    ImageBuffer m_qrCodeImageBuffer;
    std::thread m_thread;
    bool m_isWatcherRunning;

};