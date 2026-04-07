/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include <iostream>
#include <fstream>
#include <thread>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <utility>
#include <glm/vec2.hpp>

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

#include "fonts/bdfont-support.h"
#include "fonts/font-terminus_12n.h" // menu item monospaced 
#include "fonts/font-terminus_14n.h"
#include "fonts/font-terminus_16n.h" 
#include "fonts/font-terminus_16b.h"
#include "fonts/font-terminus_18n.h" // menu item
#include "fonts/font-terminus_20n.h"
#include "fonts/font-terminus_22n.h" 
#include "fonts/font-terminus_24n.h"
#include "fonts/font-terminus_24b.h" // menu item 
#include "fonts/font-terminus_28n.h"
#include "fonts/font-terminus_28b.h"
#include "fonts/font-terminus_32n.h" // root menu item

#include "ImageBuffer.h"


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
    Color(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0)
    {
        this->r = r;
        this->g = g;
        this->b = b;
    }
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    bool operator==(const Color&) const = default;
};

namespace COLOR
{
    static Color WHITE = {255, 255, 255};
    static Color GREY= {127, 127, 127};
    static Color BLACK = {0, 0, 0};
    static Color RED = {255, 0, 0};
    static Color DARK_RED = {100, 0, 0};
    static Color GREEN = {0, 255, 0};
    static Color DARK_GREEN = {0, 100, 0};
    static Color BLUE = {0, 0, 255};
    static Color DARK_BLUE = {0, 0, 100};
    static Color YELLOW = {255, 255, 0};
    static Color MAGENTA = {255, 0, 255};
    static Color CYAN = {0, 255, 255};
    static Color FOREGROUND = COLOR::WHITE;
    static Color INVERTED = COLOR::BLACK;
    static Color PLANE_0 = {0, 183, 150};
    static Color PLANE_1 = {255, 214, 53};
    static Color PLANE_2 = {255, 95, 162};
    static Color PLANE_3 = {138, 105, 212};
};

enum class AnchorPoint {    
    LEFT_TOP,           // X_Y
    CENTER_TOP,
    // RIGHT_TOP,

    // LEFT_CENTER,
    CENTER_CENTER //,
    // RIGHT_CENTER,

    // LEFT_BOTTOM,
    // CENTER_BOTTOM,
    // RIGHT_BOTTOM
};

enum class TextAlign {
    LEFT,
    CENTER //,
    // RIGHT
};

struct DrawStyle
{
    Color color = COLOR::WHITE;
    bool isFilled = false;
    int strokeWidth = 1;
    AnchorPoint anchorPoint = AnchorPoint::LEFT_TOP;
};

namespace BDF {
    struct BdfEmitCtx {
        Image* img = nullptr;
        int baseX = 0;
        int baseY = 0;
        uint8_t stripe = 0;
        Color color = COLOR::WHITE;
    };
    extern "C" void bdf_on_stripe(uint8_t stripe, uint8_t width, void* userdata);
    extern "C" void bdf_on_emit(uint8_t x, uint8_t bits, void* userdata);

    struct TextStyle {
        const ::FontData* font;
        TextAlign align = TextAlign::LEFT;
        Color color = COLOR::WHITE;
        int lineHeight = 16;

    };

    namespace TEXTSTYLE
    {
        static TextStyle MENU_TITLE =           { .font = &font_terminus_24b, .align = TextAlign::CENTER, .lineHeight = 16 };
        static TextStyle ROOT_MENU_ITEM =       { .font = &font_terminus_32n, .align = TextAlign::CENTER, .lineHeight = 20 };
        static TextStyle MENU_ITEM =            { .font = &font_terminus_18n,                             .lineHeight = 14 };
        static TextStyle MENU_ITEM_MONOSPACED = { .font = &font_terminus_12n,                             .lineHeight = 11 };
    };    
} 

class StbRenderer
{
private:
    Image m_img;
    std::queue<Image> m_imageQueue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_isEnabled = true;
    std::pair<glm::vec2, glm::vec2> m_boundingBox;   // two points: top left, bottom right
    float m_boundingBoxOpacity;

    void queueCurrentImage();

public:
    StbRenderer();  // call init() after use
    ~StbRenderer();
    
    void init(int width, int height);
    
    int width() const;
    int height() const;

    void setEnabled(bool enabled);
    bool isEnabled() const;

    void clear();
    void update();
    Image popImage();

    // image handling
    void savePNG(const std::string& filename);
    void drawImage(const ImageBuffer& imageBuffer, int posX = 0, int posY = 0);
    void drawImageNEW(const ImageBuffer& imageBuffer, glm::vec2 pos);
    void drawSubImage(const ImageBuffer& imageBuffer, glm::uvec2 destPos, glm::uvec2 srcPos, glm::uvec2 srcSize);

    // basic shapes, old->delete:
    void drawLine(int x0, int y0, int x1, int y1, Color color = COLOR::WHITE, int thickness = 1);
    void drawRect(int x0, int y0, int w, int h, Color color = COLOR::WHITE);
    void drawEmptyRect(int x0, int y0, int w, int h, Color color = COLOR::WHITE, int thickness = 1);
    void drawEmptyCenteredRect(int x0, int y0, int w, int h, Color color = COLOR::WHITE, float thickness = 1.0f);
    void drawEmptyPolygon(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, Color c = COLOR::WHITE, float thickness = 1.0f);
    // void drawPolygon(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, Color c = COLOR::WHITE);
    // void drawFilledTriangle(int x0, int y0, int x1, int y1, int x2, int y2,Color c = COLOR::WHITE);
    // void drawArrow(int x0, int y0, int size, int direction, Color color = COLOR::WHITE);
    
    // basic shapes, new versions (WIP):
    void drawLineNEW(glm::vec2 from, glm::vec2 to, DrawStyle drawStyle);
    void drawRectNEW(glm::vec2 pos, glm::vec2 size, DrawStyle drawStyle);
    void drawTriangleNEW(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, DrawStyle drawStyle);
    void drawPolygonNEW(std::vector<glm::vec2> pos, DrawStyle drawStyle);

    // set/reset bounding box for drawing calls:
    void setBoundingBox(glm::vec2 pos, glm::vec2 size, AnchorPoint anchorPoint, float opacity = 0.0f);
    void resetBoundingBox();
    bool insideBoundingBox(glm::vec2 pos) const;
    void setPixelClipped(glm::vec2 pos, Color c);
    void blendPixelClipped(glm::vec2 pos, Color c, uint8_t srcAlpha);
    void roundVec2(glm::vec2& pos);

    // font functions:
    void drawTextBdf(const std::string& text, glm::uvec2 pos,BDF::TextStyle textStyle);
    int getFontLineHeight(BDF::TextStyle textStyle);
    int getTextWidth(const std::string& text, const BDF::TextStyle& textStyle);
};