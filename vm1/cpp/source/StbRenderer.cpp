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
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            setPixel(x, y, r, g, b);
}

StbRenderer::StbRenderer() : m_img(1, 1)  // Default: 1x1 placeholder
{
    // Font loading will happen in init()
}

StbRenderer::StbRenderer(int width, int height) : m_img(width, height)
{
    clear();

    if (!loadFont("fonts/" + fontNameMonospaced))
    {
        std::cerr << "Font load failed" << std::endl;
    }
    std::cout << "StbRenderer Constructor: Renderer initialized with " << width << "x" << height << std::endl;
}

void StbRenderer::init(int width, int height)
{
    // Recreate the image with new dimensions using placement new
    m_img.~Image();
    new (&m_img) Image(width, height);
    
    clear();

    if (!loadFont("fonts/" + fontNameMonospaced))
    {
        std::cerr << "Font load failed" << std::endl;
    }
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

void StbRenderer::drawLine(int x0, int y0, int x1, int y1, Color color) 
{
    // Clamp endpoints first
    x0 = std::max(0, std::min(x0, m_img.width-1));
    y0 = std::max(0, std::min(y0, m_img.height-1));
    x1 = std::max(0, std::min(x1, m_img.width-1));
    y1 = std::max(0, std::min(y1, m_img.height-1));
    
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        m_img.setPixel(x0, y0, color.r, color.g, color.b);
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

void StbRenderer::drawArrow(int x0, int y0, int s, int direction, Color color)
{
    if (!m_isEnabled) return;
    switch (direction){
        case 0: // up
            drawLine(x0, y0 - s/2, x0, y0 + s/2);
            drawLine(x0, y0 - s/2, x0 + s/2/3, y0 - s/2/3);
            drawLine(x0, y0 - s/2, x0 - s/2/3, y0 - s/2/3);
        break;
        case 1: // right
            drawLine(x0 - s/2, y0, x0 + s/2, y0);
            drawLine(x0 + s/2, y0, x0 + s/2/3, y0 + s/2/3);
            drawLine(x0 + s/2, y0, x0 + s/2/3, y0 - s/2/3);
        break;
        case 2: // down
            drawLine(x0, y0 - s/2, x0, y0 + s/2);
            drawLine(x0, y0 + s/2, x0 + s/2/3, y0 + s/2/3);
            drawLine(x0, y0 + s/2, x0 - s/2/3, y0 + s/2/3);
        break;
        case 3: // left
            drawLine(x0 - s/2, y0, x0 + s/2, y0);
            drawLine(x0 - s/2, y0, x0 - s/2/3, y0 + s/2/3);
            drawLine(x0 - s/2, y0, x0 - s/2/3, y0 - s/2/3);
        break;
        default:
        break;
    }
}

void StbRenderer::drawEmptyRect(int x0, int y0, int w, int h, Color color)
{
    if (!m_isEnabled) return;

    for (int x = x0; x <= x0 + w; ++x)
    {
        m_img.setPixel(x, y0, color.r, color.g, color.b);
        m_img.setPixel(x, y0 + h, color.r, color.g, color.b);
    }
    for (int y = y0; y <= y0 + h; ++y)
    {
        m_img.setPixel(x0, y, color.r, color.g, color.b);
        m_img.setPixel(x0 + w, y, color.r, color.g, color.b);
    }
}

void StbRenderer::drawEmptyCenteredRect(int x0, int y0, int w, int h, Color color) 
{
    int leftX = x0 - w / 2;
    int topY = y0 - h / 2;
    drawEmptyRect(leftX, topY, w, h, color);
}

void StbRenderer::drawPolygon(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, Color c) 
{
    drawFilledTriangle(x0,y0, x1,y1, x2,y2, c);
    drawFilledTriangle(x0,y0, x2,y2, x3,y3, c);
}

void StbRenderer::drawEmptyPolygon(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, Color c) 
{
    drawLine(x0, y0, x1, y1, c);
    drawLine(x1, y1, x2, y2, c);
    drawLine(x2, y2, x3, y3, c);
    drawLine(x3, y3, x0, y0, c);
}

void StbRenderer::drawFilledTriangle(
    int x0, int y0,
    int x1, int y1,
    int x2, int y2,
    Color c)
{
    auto swap = [](int& a, int& b){ int t=a; a=b; b=t; };

    // sort vertices by y
    if (y0 > y1) { swap(y0,y1); swap(x0,x1); }
    if (y1 > y2) { swap(y1,y2); swap(x1,x2); }
    if (y0 > y1) { swap(y0,y1); swap(x0,x1); }

    auto drawScanline = [&](int y, int xa, int xb)
    {
        if (y < 0 || y >= m_img.height) return;

        if (xa > xb) swap(xa, xb);

        xa = std::max(xa,0);
        xb = std::min(xb,m_img.width-1);

        for(int x = xa; x <= xb; x++)
            m_img.setPixel(x,y,c.r,c.g,c.b);
    };

    auto edgeInterp = [](int y0,int x0,int y1,int x1,int y) -> int
    {
        if (y1 == y0) return x0;
        return (int)(x0 + (float)(x1-x0)*(y-y0)/(float)(y1-y0));
    };

    // bottom-flat part
    for(int y = y0; y <= y1; y++)
    {
        int xa = edgeInterp(y0,x0,y2,x2,y);
        int xb = edgeInterp(y0,x0,y1,x1,y);
        drawScanline(y, xa, xb);
    }

    // top-flat part
    for(int y = y1; y <= y2; y++)
    {
        int xa = edgeInterp(y0,x0,y2,x2,y);
        int xb = edgeInterp(y1,x1,y2,x2,y);
        drawScanline(y, xa, xb);
    }
}

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

void StbRenderer::savePNG(const std::string& filename)
{
    stbi_write_png(filename.c_str(), m_img.width, m_img.height, 3, m_img.pixels.data(), m_img.width * 3);
}

bool StbRenderer::loadFont(const std::string& fontPath)
{
    std::ifstream file(fontPath, std::ios::binary);
    if (!file)
    {
        std::cerr << "Error: Cannot open font file: " << fontPath << "\n";
        return false;
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0);
    fontBuffer.resize(size);
    file.read((char *)fontBuffer.data(), size);

    if (!stbtt_InitFont(&font, fontBuffer.data(), 0))
    {
        std::cerr << "Error: stbtt_InitFont failed\n";
        return false;
    }
    return true;
}

void StbRenderer::drawText(const std::string& text, int posX, int posY, float fontSize, Color color)
{
    if (!m_isEnabled) return;

    float scale = stbtt_ScaleForMappingEmToPixels(&font, fontSize);

    // Get font metrics
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
    int baseline = (int)(ascent * scale);
    // std::cout << "scale: " << scale
    //           << " ascent: " << ascent
    //           << " descent: " << descent
    //           << " baseLine: " << baseline
    //           << " lineGap: " << lineGap << std::endl;

    for (char c : text)
    {
        int width, height, xoff, yoff;
        // TODO: We need some caching eg. create a character map with these bitmaps.
        unsigned char *bitmap = stbtt_GetCodepointBitmap(&font, 0, scale, c, &width, &height, &xoff, &yoff);

        //std::cout << "Codepoint: " << c << " width: " << width << " height: " << height << " xoff: " << xoff << " yoff: " << yoff;

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                uint8_t value = bitmap[y * width + x]; // 0..255
                if (value == 0)
                    continue;
                // uint8_t r_ = (uint8_t)((float)color.r / 255.0) * (float)value;
                // uint8_t g_ = (uint8_t)((float)color.g / 255.0) * (float)value;
                // uint8_t b_ = (uint8_t)((float)color.b / 255.0) * (float)value;
                // m_img.setPixel(x + posX + xoff, y + posY + yoff + baseline, r_, g_, b_);
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
        stbtt_GetCodepointHMetrics(&font, c, &advanceWidth, &leftSideBearing);
        // std::cout << " advanceWidth: " << (float)advanceWidth * scale
        //           << " leftSideBearing: " << (float)leftSideBearing * scale << std::endl;
        posX += advanceWidth * scale;

        stbtt_FreeBitmap(bitmap, nullptr);
    }
}

int StbRenderer::getFontLineHeight(float fontSize)
{
    float scale = stbtt_ScaleForMappingEmToPixels(&font, fontSize);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
    int baseline = (int)(ascent * scale);
    return baseline + 3; // '+3' is just a 'random' value for aesthetic purpose
}

int StbRenderer::getTextWidth(const std::string& text, float fontSize)
{
    float scale = stbtt_ScaleForMappingEmToPixels(&font, fontSize);
    int textWidth = 0;
    for (char c : text)
    {
        int width, height, xoff, yoff;
        int advanceWidth, leftSideBearing;
        stbtt_GetCodepointHMetrics(&font, c, &advanceWidth, &leftSideBearing);
        textWidth += advanceWidth * scale;
    }

    return textWidth;
}