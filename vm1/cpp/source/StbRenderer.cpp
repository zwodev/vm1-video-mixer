#include "StbRenderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

void Image::setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    if (x < 0 || x >= width || y < 0 || y >= height)
        return;
    size_t idx = 3 * (y * width + x);
    pixels[idx] = r;
    pixels[idx + 1] = g;
    pixels[idx + 2] = b;
}

// Blend pixel with alpha from 0-255
void Image::blendPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha)
{
    if (x < 0 || x >= width || y < 0 || y >= height)
        return;
    size_t idx = 3 * (y * width + x);
    float a = alpha / 255.f;
    pixels[idx] = static_cast<uint8_t>(pixels[idx] * (1 - a) + r * a);
    pixels[idx + 1] = static_cast<uint8_t>(pixels[idx + 1] * (1 - a) + g * a);
    pixels[idx + 2] = static_cast<uint8_t>(pixels[idx + 2] * (1 - a) + b * a);
}

void Image::clear(uint8_t r, uint8_t g, uint8_t b)
{
    // for (int y = 0; y < height; ++y)
    //     for (int x = 0; x < width; ++x)
    //         setPixel(x, y, r, g, b);

    // TODO: How can we have different colors without impacting performance too much?
    memset(pixels.data(), 0, pixels.size());
    //std::fill(pixels.begin(), pixels.end(), 0);
}

StbRenderer::StbRenderer() : m_img(1, 1)  // Default: 1x1 placeholder
{
    // Font loading will happen in init()
}

// StbRenderer::StbRenderer(int width, int height) : m_img(width, height)
// {
//     clear();
//     if (!loadFont("fonts/" + fontNameMonospaced))
//     {
//         std::cerr << "Font load failed" << std::endl;
//     }
//     std::cout << "StbRenderer Constructor: Renderer initialized with " << width << "x" << height << std::endl;
// }

void StbRenderer::init(int width, int height)
{
    m_img.~Image();
    new (&m_img) Image(width, height);
    
    clear();

    for (const std::string& filename : m_fontFilenames)
    {
        FONT::FontData fontData;
        fontData.filename = filename;
        m_fontData.emplace(filename, std::move(fontData));
    }
    for (auto& filenameDataPair : m_fontData)
    {
        if (!loadFont(filenameDataPair.second))
            std::cerr << "Font load failed: fonts/" + filenameDataPair.first << std::endl;
    }
    resetBoundingBox();
    std::cout << "StbRenderer Init: Renderer initialized with " << width << "x" << height << std::endl;
}

StbRenderer::~StbRenderer()
{
    // stbtt_FreeBitmap(bitmap, nullptr);
}

int StbRenderer::width() const
{
    return m_img.width;
}

int StbRenderer::height() const
{
    return m_img.height;
}

void StbRenderer::setEnabled(bool enabled)
{
    m_isEnabled = enabled;
}

bool StbRenderer::isEnabled() const 
{
    return m_isEnabled;
}

void StbRenderer::clear()
{
    m_img.clear();
}

void StbRenderer::update()
{
    queueCurrentImage();   
}

void StbRenderer::queueCurrentImage()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // Clear old frames - always show the latest frame
    // This prevents queue backup when main loop is faster than display
    while (!m_imageQueue.empty()) {
        m_imageQueue.pop();
    }
    m_imageQueue.push(m_img);
    m_cv.notify_one();
}

Image StbRenderer::popImage()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this] { return !m_imageQueue.empty(); });
    Image imageBuffer = m_imageQueue.front();
    m_imageQueue.pop();
    return imageBuffer;
}

// void StbRenderer::drawLine(int x0, int y0, int x1, int y1, Color color, int thickness) 
// {
//     // Clamp endpoints first
//     x0 = std::max(0, std::min(x0, m_img.width-1));
//     y0 = std::max(0, std::min(y0, m_img.height-1));
//     x1 = std::max(0, std::min(x1, m_img.width-1));
//     y1 = std::max(0, std::min(y1, m_img.height-1));
    
//     int dx = abs(x1 - x0);
//     int dy = abs(y1 - y0);
//     int sx = x0 < x1 ? 1 : -1;
//     int sy = y0 < y1 ? 1 : -1;
//     int err = dx - dy;
    
//     while (true) {
//         m_img.setPixel(x0, y0, color.r, color.g, color.b);
//         if (x0 == x1 && y0 == y1) break;
        
//         int e2 = 2 * err;
//         if (e2 > -dy) {
//             err -= dy;
//             x0 += sx;
//         }
//         if (e2 < dx) {
//             err += dx;
//             y0 += sy;
//         }
//     }
// }
void StbRenderer::drawLine(int x0, int y0, int x1, int y1, Color color, int thickness)
{
    if (!m_isEnabled) return;

    thickness = std::max(1, thickness);
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    while (true) {
        if (x0 >= 0 && x0 < m_img.width && y0 >= 0 && y0 < m_img.height) {
            if (thickness == 1) {
                m_img.setPixel(x0, y0, color.r, color.g, color.b);
            } else {
                int half = thickness / 2;
                int left = x0 - half;
                int top = y0 - half;
                for (int oy = 0; oy < thickness; ++oy)
                    for (int ox = 0; ox < thickness; ++ox)
                        m_img.setPixel(left + ox, top + oy, color.r, color.g, color.b);
            }
        }
        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}


void StbRenderer::drawRect(int x0, int y0, int w, int h, Color color)
{
    if (!m_isEnabled) return;

    for (int y = y0; y <= y0 + h; ++y)
        for (int x = x0; x <= x0 + w; ++x)
            m_img.setPixel(x, y, color.r, color.g, color.b);
}

// void StbRenderer::drawArrow(int x0, int y0, int s, int direction, Color color)
// {
//     if (!m_isEnabled) return;
//     switch (direction){
//         case 0: // up
//             drawLine(x0, y0 - s/2, x0, y0 + s/2);
//             drawLine(x0, y0 - s/2, x0 + s/2/3, y0 - s/2/3);
//             drawLine(x0, y0 - s/2, x0 - s/2/3, y0 - s/2/3);
//         break;
//         case 1: // right
//             drawLine(x0 - s/2, y0, x0 + s/2, y0);
//             drawLine(x0 + s/2, y0, x0 + s/2/3, y0 + s/2/3);
//             drawLine(x0 + s/2, y0, x0 + s/2/3, y0 - s/2/3);
//         break;
//         case 2: // down
//             drawLine(x0, y0 - s/2, x0, y0 + s/2);
//             drawLine(x0, y0 + s/2, x0 + s/2/3, y0 + s/2/3);
//             drawLine(x0, y0 + s/2, x0 - s/2/3, y0 + s/2/3);
//         break;
//         case 3: // left
//             drawLine(x0 - s/2, y0, x0 + s/2, y0);
//             drawLine(x0 - s/2, y0, x0 - s/2/3, y0 + s/2/3);
//             drawLine(x0 - s/2, y0, x0 - s/2/3, y0 - s/2/3);
//         break;
//         default:
//         break;
//     }
// }

void StbRenderer::drawEmptyRect(int x0, int y0, int w, int h, Color color, int thickness)
{
    if (!m_isEnabled) return;

    drawLine(x0, y0, x0 + w, y0, color, thickness);
    drawLine(x0 + w, y0, x0 + w, y0 + h, color, thickness);
    drawLine(x0, y0 + h, x0 + w, y0 + h, color, thickness);
    drawLine(x0, y0, x0, y0 + h, color, thickness);


    // for (int x = x0; x <= x0 + w; ++x)
    // {
    //     m_img.setPixel(x, y0, color.r, color.g, color.b);
    //     m_img.setPixel(x, y0 + h, color.r, color.g, color.b);
    // }
    // for (int y = y0; y <= y0 + h; ++y)
    // {
    //     m_img.setPixel(x0, y, color.r, color.g, color.b);
    //     m_img.setPixel(x0 + w, y, color.r, color.g, color.b);
    // }
}

void StbRenderer::drawEmptyCenteredRect(int x0, int y0, int w, int h, Color color, float thickness) 
{
    int leftX = x0 - w / 2;
    int topY = y0 - h / 2;
    drawEmptyRect(leftX, topY, w, h, color, thickness);
}

// void StbRenderer::drawPolygon(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, Color c) 
// {
//     drawFilledTriangle(x0,y0, x1,y1, x2,y2, c);
//     drawFilledTriangle(x0,y0, x2,y2, x3,y3, c);
// }

void StbRenderer::drawEmptyPolygon(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, Color color, float thickness) 
{
    drawLine(x0, y0, x1, y1, color, thickness);
    drawLine(x1, y1, x2, y2, color, thickness);
    drawLine(x2, y2, x3, y3, color, thickness);
    drawLine(x3, y3, x0, y0, color, thickness);
}

// void StbRenderer::drawFilledTriangle(
//     int x0, int y0,
//     int x1, int y1,
//     int x2, int y2,
//     Color c)
// {
//     auto swap = [](int& a, int& b){ int t=a; a=b; b=t; };

//     // sort vertices by y
//     if (y0 > y1) { swap(y0,y1); swap(x0,x1); }
//     if (y1 > y2) { swap(y1,y2); swap(x1,x2); }
//     if (y0 > y1) { swap(y0,y1); swap(x0,x1); }

//     auto drawScanline = [&](int y, int xa, int xb)
//     {
//         if (y < 0 || y >= m_img.height) return;

//         if (xa > xb) swap(xa, xb);

//         xa = std::max(xa,0);
//         xb = std::min(xb,m_img.width-1);

//         for(int x = xa; x <= xb; x++)
//             m_img.setPixel(x,y,c.r,c.g,c.b);
//     };

//     auto edgeInterp = [](int y0,int x0,int y1,int x1,int y) -> int
//     {
//         if (y1 == y0) return x0;
//         return (int)(x0 + (float)(x1-x0)*(y-y0)/(float)(y1-y0));
//     };

//     // bottom-flat part
//     for(int y = y0; y <= y1; y++)
//     {
//         int xa = edgeInterp(y0,x0,y2,x2,y);
//         int xb = edgeInterp(y0,x0,y1,x1,y);
//         drawScanline(y, xa, xb);
//     }

//     // top-flat part
//     for(int y = y1; y <= y2; y++)
//     {
//         int xa = edgeInterp(y0,x0,y2,x2,y);
//         int xb = edgeInterp(y1,x1,y2,x2,y);
//         drawScanline(y, xa, xb);
//     }
// }

void StbRenderer::drawImage(const ImageBuffer& imageBuffer, int posX, int posY)
{
    if (!m_isEnabled) return;

    for (int y = 0; y < imageBuffer.height; ++y)
    {
        for (int x = 0; x < imageBuffer.width; ++x)
        {
            int pixel_index = (y * imageBuffer.width + x) * imageBuffer.channels;
            unsigned char r = imageBuffer.data[pixel_index + 0];
            unsigned char g = imageBuffer.data[pixel_index + 1];
            unsigned char b = imageBuffer.data[pixel_index + 2];
            m_img.setPixel(x, y, r, g, b);
        }
    }
}

void StbRenderer::drawImageNEW(const ImageBuffer& imageBuffer, glm::vec2 pos)
{
    if (!m_isEnabled) return;
    if (!imageBuffer.isValid || imageBuffer.data == nullptr) return;
    if (imageBuffer.channels < 3) return; // need at least RGB

    const int w = imageBuffer.width;
    const int h = imageBuffer.height;
    const int ch = imageBuffer.channels;

    roundVec2(pos);

    const auto* bytes = reinterpret_cast<const unsigned char*>(imageBuffer.data);

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const int src = (y * w + x) * ch;

            uint8_t r = bytes[src + 0];
            uint8_t g = bytes[src + 1];
            uint8_t b = bytes[src + 2];
            uint8_t a = (ch >= 4) ? bytes[src + 3] : 255;

            if (a == 0) continue;

            blendPixelClipped(glm::vec2(pos.x + x, pos.y + y), Color{r, g, b}, a);
        }
    }
}

void StbRenderer::drawAnimatedSprite(const ImageBuffer& imageBuffer, int frameIndex, glm::vec2 pos)
{

}



void StbRenderer::savePNG(const std::string& filename)
{
    stbi_write_png(filename.c_str(), m_img.width, m_img.height, 3, m_img.pixels.data(), m_img.width * 3);
}

bool StbRenderer::loadFont(FONT::FontData& fontData)
{
    std::string path = "fonts/" + fontData.filename;
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        std::cerr << "Error: Cannot open font file: " << path << "\n";
        return false;
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0);
    fontData.fontBuffer.resize(size);
    file.read((char *)fontData.fontBuffer.data(), size);

    if (!stbtt_InitFont(&fontData.font, fontData.fontBuffer.data(), 0))
    {
        std::cerr << "Error: stbtt_InitFont failed\n";
        return false;
    }
    return true;
}

void StbRenderer::drawText(const std::string& text, int posX, int posY, FONT::TextStyle textStyle, Color color)
{
    if (!m_isEnabled) return;
    FONT::FontData& fontData = m_fontData[textStyle.fontName];  
    float scale = stbtt_ScaleForMappingEmToPixels(&fontData.font, textStyle.size);

    // Get font metrics
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontData.font, &ascent, &descent, &lineGap);
    int baseline = (int)(ascent * scale);
    // std::cout << "scale: " << scale
    //           << " ascent: " << ascent
    //           << " descent: " << descent
    //           << " baseLine: " << baseline
    //           << " lineGap: " << lineGap << std::endl;

    if(textStyle.align == FONT::TextStyle::Align::CENTER) {
        int textWidth = getTextWidth(text, textStyle);
        posX -= textWidth / 2;
    }

    for (char c : text)
    {
        int width, height, xoff, yoff;
        // TODO: We need some caching eg. create a character map with these bitmaps.
        unsigned char *bitmap = stbtt_GetCodepointBitmap(&fontData.font, 0, scale, c, &width, &height, &xoff, &yoff);

        //std::cout << "Codepoint: " << c << " width: " << width << " height: " << height << " xoff: " << xoff << " yoff: " << yoff;

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                uint8_t value = bitmap[y * width + x]; // 0..255
                if (value == 0)
                    continue;
                m_img.blendPixel(
                    x + posX + xoff,
                    y + posY + yoff + baseline,
                    color.r,
                    color.g,
                    color.b,
                    value
                );                
            }
        }

        int advanceWidth, leftSideBearing;
        stbtt_GetCodepointHMetrics(&fontData.font, c, &advanceWidth, &leftSideBearing);
        // std::cout << " advanceWidth: " << (float)advanceWidth * scale
        //           << " leftSideBearing: " << (float)leftSideBearing * scale << std::endl;
        posX += advanceWidth * scale;

        stbtt_FreeBitmap(bitmap, nullptr);
    }
}

int StbRenderer::getFontLineHeight(FONT::TextStyle textStyle)
{
    FONT::FontData& fontData = m_fontData[textStyle.fontName];  

    float scale = stbtt_ScaleForMappingEmToPixels(&fontData.font, textStyle.size);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontData.font, &ascent, &descent, &lineGap);
    int baseline = (int)(ascent * scale);
    return baseline + 3; // '+3' is just a 'random' value for aesthetic purpose
}

int StbRenderer::getTextWidth(const std::string& text, FONT::TextStyle textStyle)
{
    FONT::FontData& fontData = m_fontData[textStyle.fontName];  

    float scale = stbtt_ScaleForMappingEmToPixels(&fontData.font, textStyle.size);
    int textWidth = 0;
    for (char c : text)
    {
        int width, height, xoff, yoff;
        int advanceWidth, leftSideBearing;
        stbtt_GetCodepointHMetrics(&fontData.font, c, &advanceWidth, &leftSideBearing);
        textWidth += advanceWidth * scale;
    }

    return textWidth;
}

void StbRenderer::drawLineNEW(glm::vec2 from, glm::vec2 to, DrawStyle drawStyle)
{
    if (!m_isEnabled) return;

    const int strokeWidth = std::max(1, drawStyle.strokeWidth);
    // Snap float endpoints to pixel grid once.
    int x0 = static_cast<int>(std::round(from.x));
    int y0 = static_cast<int>(std::round(from.y));
    int x1 = static_cast<int>(std::round(to.x));
    int y1 = static_cast<int>(std::round(to.y));
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    while (true) {
        if (x0 >= 0 && x0 < m_img.width && y0 >= 0 && y0 < m_img.height) {
            if (strokeWidth == 1) {
                setPixelClipped(glm::vec2(x0, y0), drawStyle.color);
            } else {
                const int half = strokeWidth / 2;
                for (int oy = 0; oy < strokeWidth; ++oy) {
                    for (int ox = 0; ox < strokeWidth; ++ox) {
                        setPixelClipped(glm::vec2(x0 + ox - half, y0 + oy - half), drawStyle.color);
                    }
                }
            }
        }
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

void StbRenderer::drawRectNEW(glm::vec2 pos, glm::vec2 size, DrawStyle drawStyle) 
{
    roundVec2(pos);
    roundVec2(size);
    if (!m_isEnabled) return;
    glm::vec2 topLeft = pos;
    if (drawStyle.anchorPoint == AnchorPoint::CENTER) {
        topLeft = pos - size * 0.5f;
    }

    if (drawStyle.isFilled) {
        for (int y = topLeft.y; y < topLeft.y + size.y; ++y)
            for (int x = topLeft.x; x < topLeft.x + size.x; ++x)
                setPixelClipped(glm::vec2(x, y), drawStyle.color);
    } 
    else 
    {  
        drawLineNEW(topLeft, topLeft + glm::vec2(size.x, 0), drawStyle);
        drawLineNEW(topLeft + glm::vec2(size.x, 0), topLeft + size, drawStyle);
        drawLineNEW(topLeft + glm::vec2(0, size.y), topLeft + size, drawStyle);
        drawLineNEW(topLeft, topLeft + glm::vec2(0, size.y), drawStyle);
    }
}

void StbRenderer::drawTriangleNEW(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, DrawStyle drawStyle)
{

}

void StbRenderer::drawPolygonNEW(std::vector<glm::vec2> pos, DrawStyle drawStyle)
{
    if (!m_isEnabled) return;
    for (int i = 0; i < pos.size(); i++) {
        roundVec2(pos[i]);
        drawLineNEW(pos[i], pos[(i + 1) % pos.size()], drawStyle);
    }
}

void StbRenderer::setBoundingBox(glm::vec2 pos, glm::vec2 size, AnchorPoint anchorPoint, float opacity)
{
    m_boundingBoxOpacity = opacity;
    if (anchorPoint == AnchorPoint::TOP_LEFT) {
        m_boundingBox.first = pos;
        m_boundingBox.second = pos + size;
    } else if (anchorPoint == AnchorPoint::CENTER) {
        m_boundingBox.first = glm::vec2(pos.x - size.x / 2, pos.y - size.y / 2);
        m_boundingBox.second = glm::vec2(pos.x + size.x / 2, pos.y + size.y / 2);
    }

    roundVec2(m_boundingBox.first);
    roundVec2(m_boundingBox.second);

    // extend bounding box by 1 pixel
    m_boundingBox.first -= 1.0f;
    m_boundingBox.second += 1.0f;

    // debug: draw bounding box
    // DrawStyle drawStyleBoundingBox = DrawStyle{COLOR::RED, false, 1, AnchorPoint::TOP_LEFT}; 
    // drawRectNEW(m_boundingBox.first, m_boundingBox.second - m_boundingBox.first, drawStyleBoundingBox);
}

void StbRenderer::resetBoundingBox()
{
    m_boundingBox.first = glm::vec2(-1.0f, -1.0f);
    m_boundingBox.second = glm::vec2(m_img.width, m_img.height);
}

inline bool StbRenderer::insideBoundingBox(glm::vec2 pos) const {
    return pos.x > m_boundingBox.first.x &&
           pos.x <  m_boundingBox.second.x &&
           pos.y > m_boundingBox.first.y &&
           pos.y <  m_boundingBox.second.y;
}

inline void StbRenderer::setPixelClipped(glm::vec2 pos, Color c) {
    roundVec2(pos);

    if (m_boundingBoxOpacity == 0.0f)
    {
        if (!insideBoundingBox(pos)) 
            return;
        else
            m_img.setPixel(pos.x, pos.y, c.r, c.g, c.b);

    } 
    else
    {
        if(!insideBoundingBox(pos)) {
            m_img.blendPixel(pos.x, pos.y, c.r, c.g, c.b, std::round(m_boundingBoxOpacity * 255.0f));
        } else 
        {
            m_img.setPixel(pos.x, pos.y, c.r, c.g, c.b);
        }
    }
}

inline void StbRenderer::blendPixelClipped(glm::vec2 pos, Color c, uint8_t srcAlpha)
{
    if (srcAlpha == 0) return;

    roundVec2(pos);

    // boundingBoxOpacity:
    // - 0.0f means "only draw inside the bounding box"
    // - >0 means "outside the bounding box is dimmed" (we scale PNG alpha)
    if (m_boundingBoxOpacity == 0.0f)
    {
        if (!insideBoundingBox(pos)) return;

        // Inside: use PNG-provided alpha
        if (srcAlpha == 255)
            m_img.setPixel(pos.x, pos.y, c.r, c.g, c.b);
        else
            m_img.blendPixel(pos.x, pos.y, c.r, c.g, c.b, srcAlpha);
    }
    else
    {
        uint8_t effectiveAlpha = srcAlpha;

        if (!insideBoundingBox(pos))
        {
            const uint8_t bboxAlpha255 = static_cast<uint8_t>(std::round(m_boundingBoxOpacity * 255.0f));
            // Scale alpha outside the box by boundingBoxOpacity (integer math)
            effectiveAlpha = static_cast<uint8_t>((static_cast<int>(srcAlpha) * bboxAlpha255 + 127) / 255);
            if (effectiveAlpha == 0) return;
        }

        if (effectiveAlpha == 255)
            m_img.setPixel(pos.x, pos.y, c.r, c.g, c.b);
        else
            m_img.blendPixel(pos.x, pos.y, c.r, c.g, c.b, effectiveAlpha);
    }
}

inline void StbRenderer::roundVec2(glm::vec2& pos)
{
    pos.x = std::round(pos.x);
    pos.y = std::round(pos.y);
}