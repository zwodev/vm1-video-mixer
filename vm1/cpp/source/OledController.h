/*
 * Copyright (c) 2025 Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include "oled/OLED_1in5_rgb.h"
#include "oled/DEV_Config.h"
#include "oled/GUI_Paint.h"
#include "oled/GUI_BMPfile.h"

class OledController
{
public:
    UBYTE *oledImage;

    OledController();
    ~OledController();

    static void Handler(int signo);
    int initializeOled();
    int initializeImageBuffer();

    void drawTestBMP();
    void render();

private:
    UWORD imagesize;
};
