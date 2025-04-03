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

#pragma pack(push, 1) // Ensure proper struct alignment
struct BMPFileHeader
{
    uint16_t bfType = 0x4D42; // "BM"
    uint32_t bfSize;          // File size
    uint16_t bfReserved1 = 0;
    uint16_t bfReserved2 = 0;
    uint32_t bfOffBits = 70; // Pixel data offset (56-byte DIB + 16-byte masks)
};

struct BMPInfoHeaderV3
{
    uint32_t biSize = 56; // 56-byte BITMAPV3INFOHEADER
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes = 1;
    uint16_t biBitCount = 16;   // 16-bit BMP
    uint32_t biCompression = 3; // BI_BITFIELDS (16-bit)
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter = 2835;
    int32_t biYPelsPerMeter = 2835;
    uint32_t biClrUsed = 0;
    uint32_t biClrImportant = 0;
    uint32_t biRedMask = 0xF800;   // 5-bit red
    uint32_t biGreenMask = 0x07E0; // 6-bit green
    uint32_t biBlueMask = 0x001F;  // 5-bit blue
    uint32_t biAlphaMask = 0x0000; // No alpha
};
#pragma pack(pop)

OledController::OledController()
{
}

OledController::~OledController()
{
    stop();
}

void OledController::setOledUiRenderer(OledUiRenderer* oledUiRenderer)
{
    m_oledUiRenderer = oledUiRenderer;
}

void OledController::start()
{
    m_isRunning = true;
    m_thread = std::thread(&OledController::process, this);
}

void OledController::stop()
{   
    m_isRunning = false;
    m_thread.join();
}

void OledController::process()
{
    if (!m_oledUiRenderer) return;

    initializeOled();
    initializeImageBuffer();
    while (m_isRunning) {
        ImageBuffer imageBuffer = m_oledUiRenderer->popImage();
        renderToRGB565(imageBuffer, false);
        render();
    }   
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

void OledController::renderToRGB565(ImageBuffer& imageBuffer, bool saveAsBmp)
{
    int width = imageBuffer.width;
    int height = imageBuffer.height;
    // Now you can process the data
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            // Calculate the flipped y index (flip vertically)
            int flippedY = height - 1 - y;

            // Fetch RGBA values from the texture buffer (no horizontal flip)
            uint8_t r = imageBuffer.buffer[(flippedY * width + x) * 4 + 0];
            uint8_t g = imageBuffer.buffer[(flippedY * width + x) * 4 + 1];
            uint8_t b = imageBuffer.buffer[(flippedY * width + x) * 4 + 2];

            // Convert to RGB565
            uint16_t rgb565 = (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3);

            // Store the 16-bit RGB565 into the buffer as two bytes
            oledImage[(y * width + x) * 2 + 0] = (uint8_t)(rgb565 >> 8);   // High byte (most significant byte)
            oledImage[(y * width + x) * 2 + 1] = (uint8_t)(rgb565 & 0xFF); // Low byte (least significant byte)
        }
    }

    if (saveAsBmp)
    {
        std::string filename = "output.bmp";
        // Ensure 4-byte row alignment
        int rowSize = ((width * 2 + 3) / 4) * 4;
        int dataSize = rowSize * height;
        int fileSize = sizeof(BMPFileHeader) + sizeof(BMPInfoHeaderV3) + dataSize;

        BMPFileHeader fileHeader;
        fileHeader.bfSize = fileSize;

        BMPInfoHeaderV3 infoHeader;
        infoHeader.biWidth = width;
        infoHeader.biHeight = -height; // Top-down DIB
        infoHeader.biSizeImage = dataSize;

        std::ofstream file(filename, std::ios::binary);
        if (!file)
        {
            printf("Failed to open file for writing: %s\n", filename.c_str());
            return;
        }

        // Write BMP headers
        file.write(reinterpret_cast<const char *>(&fileHeader), sizeof(fileHeader));
        file.write(reinterpret_cast<const char *>(&infoHeader), sizeof(infoHeader));

        // Write pixel data row by row (ensuring alignment)
        std::vector<uint8_t> rowBuffer(rowSize, 0);
        for (int y = 0; y < height; ++y)
        {
            memcpy(rowBuffer.data(), &oledImage[y * width * 2], width * 2);
            file.write(reinterpret_cast<const char *>(rowBuffer.data()), rowSize);
        }

        file.close();
        printf("Saved BMP: %s\n", filename.c_str());
    }
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