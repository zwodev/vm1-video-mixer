/*
 * Copyright (c) 2025 Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include <stdlib.h> //exit()
#include <signal.h> //signal()
#include "OledController.h"

OledController::OledController()
{
}

OledController::~OledController()
{
}

void OledController::Handler(int signo)
{
    // System Exit
    OLED_1in5_rgb_Clear();

    printf("\r\nHandler:exit\r\n");
    DEV_ModuleExit();

    exit(0);
}

int OledController::initializeOled()
{
    signal(SIGINT, OledController::Handler);

    if (DEV_ModuleInit() != 0)
    {
        return -1;
    }

    if (USE_IIC)
    {
        printf("Only USE_SPI, Please revise DEV_Config.h !!!\r\n");
        return -1;
    }

    OLED_1in5_rgb_Init();
    DEV_Delay_ms(50);
}

int OledController::initializeImageBuffer()
{
    imagesize = (OLED_1in5_RGB_WIDTH * 2) * OLED_1in5_RGB_HEIGHT;
    if ((oledImage = (UBYTE *)malloc(imagesize + 300)) == NULL)
    {
        printf("Failed to apply for black memory...\r\n");
        return -1;
    }
    Paint_NewImage(oledImage, OLED_1in5_RGB_WIDTH, OLED_1in5_RGB_HEIGHT, 0, BLACK);
    Paint_SetScale(65);

    printf("OLED buffer initialized with %u Bytes.\n", imagesize + 300);

    OLED_1in5_rgb_Clear();
}

void OledController::render()
{
    // printf("rendering oled\n");
    OLED_1in5_rgb_Display_chunkwise(oledImage);
}

void OledController::drawTestBMP()
{
    // 1.Select Image
    Paint_SelectImage(oledImage);
    DEV_Delay_ms(50);
    char filename[128];

    // snprintf(filename, sizeof(filename), "/home/pi/Documents/coding/vm1-support/OLED_vm1/pic/seq/frame%03d.bmp", i);
    snprintf(filename, sizeof(filename), "/home/pi/Documents/coding/vm1-video-mixer/vm1/media/img/1in5_rgb.bmp");

    GUI_ReadBmp_65K(filename, 0, 0);

    OLED_1in5_rgb_Display_chunkwise(oledImage);
    // OLED_1in5_rgb_Display(oledImage);
}