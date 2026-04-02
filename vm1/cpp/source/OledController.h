/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

// #include "OledUiRenderer.h"
#include "StbRenderer.h"

#include "oled/OLED_1in5_rgb.h"
#include "oled/DEV_Config.h"
#include "oled/GUI_Paint.h"
#include "oled/GUI_BMPfile.h"

#include <thread>

class OledController
{
public:
    OledController();
    ~OledController();

    static void Handler(int signo);
    int initializeOled();
    int initializeImageBuffer();

    void setStbRenderer(StbRenderer* stbRenderer);
    void start();
    void stop();

    void renderToRGB565(Image& imageBuffer, bool saveAsBmp);
    void render();

    const Image& getImageBuffer();

private:
    void process();

public:
    UBYTE *oledImage;

private:
    std::mutex m_mutex;
    bool m_isRunning = false;
    std::thread m_thread;
    StbRenderer* m_stbRenderer = nullptr;
    Image m_imageBuffer;
    UWORD m_imagesize;
};
