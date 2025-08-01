/*
 * Copyright (c) 2025 Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include <stdlib.h> //exit()
#include <signal.h> //signal()
#include <fstream>

#include "OledController.h"

OledController::OledController()
{
}

OledController::~OledController()
{
    stop();
}

void OledController::setStbRenderer(StbRenderer* stbRenderer)
{
    m_stbRenderer = stbRenderer;
}

void OledController::start()
{
    m_isRunning = true;
    m_thread = std::thread(&OledController::process, this);
}

void OledController::stop()
{   
    m_isRunning = false;

    // Call update to unlock the conditonal_variable.wait() in StbRenderer.
    if (m_stbRenderer) {
        m_stbRenderer->update();
    }
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void OledController::process()
{
    if (!m_stbRenderer) return;

    initializeOled();
    initializeImageBuffer();
    while (m_isRunning) {
        Image imageBuffer = m_stbRenderer->popImage();
        renderToRGB565(imageBuffer, false);
        render();
    }   

    OLED_1in5_rgb_Clear();
    DEV_ModuleExit();
    //delete oledImage;
    //oledImage = nullptr;
}

void OledController::Handler(int signo)
{
    // // System Exit
    // OLED_1in5_rgb_Clear();

    // printf("\r\nHandler:exit\r\n");
    // DEV_ModuleExit();

    //exit(0);
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
    m_imagesize = (OLED_1in5_RGB_WIDTH * 2) * OLED_1in5_RGB_HEIGHT;
    if ((oledImage = (UBYTE *)malloc(m_imagesize + 300)) == NULL)
    {
        printf("Failed to apply for black memory...\r\n");
        return -1;
    }
    Paint_NewImage(oledImage, OLED_1in5_RGB_WIDTH, OLED_1in5_RGB_HEIGHT, 0, BLACK);
    Paint_SetScale(65);

    printf("OLED buffer initialized with %u Bytes.\n", m_imagesize + 300);

    OLED_1in5_rgb_Clear();
}

void OledController::render()
{
    // printf("rendering oled\n");
    OLED_1in5_rgb_Display_chunkwise(oledImage);
}

void OledController::renderToRGB565(Image& imageBuffer, bool saveAsBmp)
{
    int width = imageBuffer.width;
    int height = imageBuffer.height;
    // Now you can process the data
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            // Calculate the flipped y index (flip vertically)
            //int flippedY = y;

            // Fetch RGBA values from the texture buffer (no horizontal flip)
            uint8_t r = imageBuffer.pixels[(y * width + x) * 3 + 0];
            uint8_t g = imageBuffer.pixels[(y * width + x) * 3 + 1];
            uint8_t b = imageBuffer.pixels[(y * width + x) * 3 + 2];

            // Convert to RGB565
            uint16_t rgb565 = (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3);

            // Store the 16-bit RGB565 into the buffer as two bytes
            oledImage[(y * width + x) * 2 + 0] = (uint8_t)(rgb565 >> 8);   // High byte (most significant byte)
            oledImage[(y * width + x) * 2 + 1] = (uint8_t)(rgb565 & 0xFF); // Low byte (least significant byte)
        }
    }
}
