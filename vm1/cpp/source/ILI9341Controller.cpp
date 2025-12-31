/*
 * Copyright (c) 2025 Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include <stdlib.h>
#include <signal.h>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <chrono>

#include "ILI9341Controller.h"

// External C functions for module init/exit
extern "C" {
    extern UBYTE ILI9341_DEV_ModuleInit(void);
    extern void ILI9341_DEV_ModuleExit(void);
}

ILI9341Controller::ILI9341Controller()
{
}

ILI9341Controller::~ILI9341Controller()
{
    stop();
}

void ILI9341Controller::setStbRenderer(StbRenderer* stbRenderer)
{
    m_stbRenderer = stbRenderer;
}

void ILI9341Controller::start()
{
    m_isRunning = true;
    m_isReady = false;
    m_thread = std::thread(&ILI9341Controller::process, this);
}

void ILI9341Controller::stop()
{
    m_isRunning = false;

    // Call update to unlock the conditional_variable.wait() in StbRenderer.
    if (m_stbRenderer) {
        m_stbRenderer->update();
    }
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void ILI9341Controller::waitForReady()
{
    while (!m_isReady) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void ILI9341Controller::process()
{
    if (!m_stbRenderer) return;

    initializeDisplay();
    initializeImageBuffer();

    m_isReady = true;

    std::cout << "ILI9341: Starting render loop\n";
    while (m_isRunning) {
        Image imageBuffer = m_stbRenderer->popImage();
        if (!m_isRunning) break;  // Check again after waking up
        convertToRGB565(imageBuffer);
        render();
    }
    std::cout << "ILI9341: Render loop ended\n";

    ILI9341_Clear(0x0000);
    ILI9341_DEV_ModuleExit();

    if (m_displayBuffer) {
        delete[] m_displayBuffer;
        m_displayBuffer = nullptr;
    }
    std::cout << "ILI9341: Cleanup complete\n";
}

void ILI9341Controller::Handler(int signo)
{
    // System Exit
    ILI9341_Clear(0x0000);

    printf("\r\nHandler:exit\r\n");
    ILI9341_DEV_ModuleExit();

    exit(0);
}

int ILI9341Controller::initializeDisplay()
{
    signal(SIGINT, ILI9341Controller::Handler);

    if (ILI9341_DEV_ModuleInit() != 0) {
        std::cerr << "Failed to initialize ILI9341 module\n";
        return -1;
    }

    ILI9341_Init();
    std::cout << "ILI9341 display initialized\n";

    return 0;
}

int ILI9341Controller::initializeImageBuffer()
{
    m_bufferSize = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2; // RGB565 = 2 bytes per pixel
    m_displayBuffer = new uint8_t[m_bufferSize];

    if (m_displayBuffer == nullptr) {
        std::cerr << "Failed to allocate display buffer\n";
        return -1;
    }

    std::memset(m_displayBuffer, 0, m_bufferSize);
    std::cout << "ILI9341 buffer initialized with " << m_bufferSize << " bytes.\n";

    // Note: Don't call ILI9341_Clear() here - the working tests don't do this
    // and it may interfere with the display state

    return 0;
}

void ILI9341Controller::render()
{
    static int renderCount = 0;
    static auto startTime = std::chrono::steady_clock::now();
    
    ILI9341_Display_nByte(m_displayBuffer, m_bufferSize);
    renderCount++;
    
    if (renderCount % 60 == 0) {  // Print every 60 frames
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
        double fps = (renderCount * 1000.0) / elapsed;
        std::cout << "ILI9341: Rendered " << renderCount << " frames (" 
                  << std::fixed << std::setprecision(1) << fps << " fps)\n";
    }
}

void ILI9341Controller::convertToRGB565(Image& imageBuffer)
{
    int srcWidth = imageBuffer.width;
    int srcHeight = imageBuffer.height;

    for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
        for (int x = 0; x < DISPLAY_WIDTH; ++x) {
            // Calculate source coordinates (nearest neighbor scaling if sizes differ)
            int srcX = (x * srcWidth) / DISPLAY_WIDTH;
            int srcY = (y * srcHeight) / DISPLAY_HEIGHT;

            // Fetch RGB values from the image buffer
            size_t srcIdx = (srcY * srcWidth + srcX) * 3;
            uint8_t r = imageBuffer.pixels[srcIdx + 0];
            uint8_t g = imageBuffer.pixels[srcIdx + 1];
            uint8_t b = imageBuffer.pixels[srcIdx + 2];

            // Convert to RGB565
            uint16_t rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);

            // Store as two bytes (big-endian for ILI9341)
            int idx = (y * DISPLAY_WIDTH + x) * 2;
            m_displayBuffer[idx + 0] = (uint8_t)(rgb565 >> 8);   // High byte
            m_displayBuffer[idx + 1] = (uint8_t)(rgb565 & 0xFF); // Low byte
        }
    }
}
