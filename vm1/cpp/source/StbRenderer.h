#pragma once

#include <iostream>
#include <fstream>
#include <thread>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>


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

struct Color
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

namespace COLOR
{
    static constexpr Color WHITE = {255, 255, 255};
    static constexpr Color BLACK = {0, 0, 0};
    static constexpr Color RED = {255, 0, 0};
    static constexpr Color FOREGROUND = COLOR::WHITE;
    static constexpr Color INVERTED = COLOR::BLACK;
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
    Image m_img;
    std::queue<Image> m_imageQueue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_isEnabled = true;

    void queueCurrentImage();

public:
    StbRenderer(int width, int height);
    ~StbRenderer();
    
    int width() const;
    int height() const;

    void setEnabled(bool enabled);
    bool isEnabled() const;

    void clear();
    void update();
    Image popImage();

    void savePNG(const std::string& filename);
    bool loadFont(const std::string& fontPath);
    void drawPng(const std::string& filename, int posX = 0, int posY = 0);
    void drawRect(int x0, int y0, int w, int h, Color color = COLOR::WHITE);
    void drawText(const std::string& text, int posX, int posY, float fontSize, Color color = COLOR::WHITE);
    int getFontLineHeight(float fontSize);
    int getTextWidth(const std::string& text, float fontSize);
};