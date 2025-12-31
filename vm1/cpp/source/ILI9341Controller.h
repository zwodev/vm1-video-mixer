/*
 * Copyright (c) 2025 Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include "StbRenderer.h"
#include "ili9341/ILI9341.h"
#include "ili9341/DEV_Config.h"

#include <thread>
#include <atomic>

class ILI9341Controller
{
public:
    static constexpr int DISPLAY_WIDTH = 320;   // Landscape mode (90° CCW rotation)
    static constexpr int DISPLAY_HEIGHT = 240;  // Landscape mode (90° CCW rotation)

    ILI9341Controller();
    ~ILI9341Controller();

    static void Handler(int signo);
    int initializeDisplay();
    int initializeImageBuffer();

    void setStbRenderer(StbRenderer* stbRenderer);
    void start();
    void stop();
    void waitForReady();

    void convertToRGB565(Image& imageBuffer);
    void render();

private:
    void process();

public:
    uint8_t* m_displayBuffer = nullptr;

private:
    bool m_isRunning = false;
    std::atomic<bool> m_isReady{false};
    std::thread m_thread;
    StbRenderer* m_stbRenderer = nullptr;
    uint32_t m_bufferSize = 0;
};
