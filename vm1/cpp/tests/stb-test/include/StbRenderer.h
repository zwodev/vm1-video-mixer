#pragma once

#include <iostream>
#include <fstream>

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "stb/stb_truetype.h"

struct Image
{
    int width, height;
    std::vector<uint8_t> pixels;

    Image(int w, int h) : width(w), height(h), pixels(w * h * 3, 0) {}
    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
    void blendPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);
    void clear(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0);
};

struct c
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct COLOR
{
    static constexpr c WHITE = {255, 255, 255};
    static constexpr c BLACK = {0, 0, 0};
    static constexpr c RED = {255, 0, 0};
    static constexpr c FOREGROUND = COLOR::WHITE;
    static constexpr c INVERTED = COLOR::BLACK;
};

/**
 * @class StbRenderer
 * @brief A simple renderer for drawing shapes and text using stb_truetype and a custom image buffer.
 *
 * This class provides basic 2D rendering functionality, including drawing rectangles,
 * rendering text using a baked font atlas, and saving the rendered image as a PNG file.
 * It uses stb_truetype for font handling and manages its own image buffer.
 *
 * @note The renderer supports ASCII characters 32..126 for text rendering.
 *
 * @author [Your Name]
 * @date [Date]
 */

class StbRenderer
{
private:
    stbtt_bakedchar cdata[96];           // ASCII 32..126
    unsigned char fontBitmap[512 * 512]; // grayscale glyph atlas
    std::vector<unsigned char> fontBuffer;
    stbtt_fontinfo font;
    std::unique_ptr<Image> img;

public:
    StbRenderer(int width, int height);
    ~StbRenderer();
    void clear();
    void savePNG(const std::string &filename);
    bool loadFont(const std::string &fontPath);
    void drawPng(const std::string &filename, int posX = 0, int posY = 0);
    void drawRect(int x0, int y0, int w, int h, c color = COLOR::WHITE);
    void drawText(const std::string &text, int posX, int posY, float fontSize, c color = COLOR::WHITE);
    int getFontLineHeight(float fontSize);
};