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
#include "stb/stb_truetype.h"

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

enum AnchorPoint {
    TOP_LEFT,
    CENTER
};
struct DrawStyle
{
    Color color = COLOR::WHITE;
    bool isFilled = false;
    int strokeWidth = 1;
    AnchorPoint anchorPoint = TOP_LEFT;
};


namespace FONT  
{
    struct FontData {
        std::vector<unsigned char> fontBuffer;
        stbtt_fontinfo font;
        std::string filename;
        // stbtt_bakedchar cdata[96];            // not needed at the moment,
        // unsigned char fontBitmap[512 * 512];  // but maybe we chache the font later...
    };

    struct TextStyle {
        enum Align {
            LEFT,
            CENTER
        };

        std::string fontName;
        float size;
        Align align = Align::LEFT;
    };

    namespace TEXTSTYLE
    {
        static TextStyle MENU_TITLE = {"CreatoDisplay-Regular.otf", 28.0f, TextStyle::Align::CENTER};
        static TextStyle ROOT_MENU_ITEM = {"CreatoDisplay-Regular.otf", 20.0f};
        static TextStyle STANDARD = {"CreatoDisplay-Regular.otf", 16.0f};
        static TextStyle LIST_ITEM = {"ProggyClean.ttf", 16.0f};
    };
};

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

    const std::vector<std::string> m_fontFilenames = {"ProggyClean.ttf", "CreatoDisplay-Regular.otf"};
    std::unordered_map<std::string, FONT::FontData> m_fontData; // filename, FontData

    void queueCurrentImage();

public:
    StbRenderer();  // call init() after use
    // StbRenderer(int width, int height);
    ~StbRenderer();
    
    void init(int width, int height);
    
    int width() const;
    int height() const;

    void setEnabled(bool enabled);
    bool isEnabled() const;

    void clear();
    void update();
    Image popImage();

    // 
    void savePNG(const std::string& filename);
    void drawImage(const ImageBuffer& imageBuffer, int posX = 0, int posY = 0);
    void drawImageNEW(const ImageBuffer& imageBuffer, glm::vec2 pos);
    // void drawAnimatedSprite(const ImageBuffer& imageBuffer, int frameIndex, glm::vec2 pos, glm::vec2 srcPos, glm::vec2 srcSize);
    void drawSubImage(const ImageBuffer& imageBuffer, glm::uvec2 destPos, glm::uvec2 srcPos, glm::uvec2 srcSize);

    
    // basic shapes:
    void drawLine(int x0, int y0, int x1, int y1, Color color = COLOR::WHITE, int thickness = 1);
    void drawRect(int x0, int y0, int w, int h, Color color = COLOR::WHITE);
    void drawEmptyRect(int x0, int y0, int w, int h, Color color = COLOR::WHITE, int thickness = 1);
    void drawEmptyCenteredRect(int x0, int y0, int w, int h, Color color = COLOR::WHITE, float thickness = 1.0f);
    // void drawPolygon(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, Color c = COLOR::WHITE);
    void drawEmptyPolygon(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, Color c = COLOR::WHITE, float thickness = 1.0f);
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
    bool loadFont(FONT::FontData& fontData);
    void drawText(const std::string& text, int posX, int posY, FONT::TextStyle textStyle, Color color = COLOR::WHITE);
    int getFontLineHeight(FONT::TextStyle textStyle);
    int getTextWidth(const std::string& text, FONT::TextStyle textStyle);
};