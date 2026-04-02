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

class MediaPool
{
public:
    MediaPool();
    ~MediaPool();

    const ImageBuffer& getLogo();

    std::vector<std::string>& getVideoFiles();
    std::string getVideoFilePath(const std::string& fileName);

    std::string getGenerativeShaderFilePath(const std::string& fileName);
    std::vector<std::string>& getGenerativeShaderFiles();
    
    std::string getEffectShaderFilePath(const std::string& fileName);
    std::vector<std::string>& getEffectShaderFiles();

    void loadQrCodeImageBuffer();
    const ImageBuffer& getQrCodeImageBuffer();

private:
    void runMediaDirectoryWatcher();
    void startDirectoryWatcher();
    void stopDirectoryWatcher();
    void updateGenerativeShaderFiles();
    void updateEffectShaderFiles();
    void updateVideoFiles();
    void updateVideoFilesPendingPreviews();
    void generateVideoFilePreview(std::string filename);

private:
    ImageBuffer m_logo = ImageBuffer("media/splash-screen.png");
    std::string m_videoFilePath = "../videos/";
    std::string m_generativeShaderPath = "../shaders/generative/";
    std::string m_effectShaderPath = "../shaders/effect/";
    std::vector<std::string> m_videoFiles;
    std::vector<std::string> m_videoFilesPreviews;
    std::vector<std::string> m_mediaFilesPendingPreview;
    std::vector<std::string> m_generativeShaderFiles;
    std::vector<std::string> m_effectShaderFiles;
    ImageBuffer m_qrCodeImageBuffer;
    std::thread m_thread;
    std::mutex m_videoMutex;
    std::mutex m_generativeShaderMutex;
    std::mutex m_effectShaderMutex;
    bool m_isWatcherRunning;

};