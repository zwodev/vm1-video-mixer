#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

struct Image
{
    int width, height;
    std::vector<uint8_t> pixels; // grayscale or RGB

    Image(int w, int h) : width(w), height(h), pixels(w * h * 3, 0) {}

    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
    {
        if (x < 0 || x >= width || y < 0 || y >= height)
            return;
        size_t idx = 3 * (y * width + x);
        pixels[idx] = r;
        pixels[idx + 1] = g;
        pixels[idx + 2] = b;
    }

    // Blend pixel with alpha from 0-255
    void blendPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha)
    {
        if (x < 0 || x >= width || y < 0 || y >= height)
            return;
        size_t idx = 3 * (y * width + x);
        float a = alpha / 255.f;
        pixels[idx] = static_cast<uint8_t>(pixels[idx] * (1 - a) + r * a);
        pixels[idx + 1] = static_cast<uint8_t>(pixels[idx + 1] * (1 - a) + g * a);
        pixels[idx + 2] = static_cast<uint8_t>(pixels[idx + 2] * (1 - a) + b * a);
    }

    void clear(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0)
    {
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
                setPixel(x, y, r, g, b);
    }
};

class SoftwareRenderer
{
public:
    SoftwareRenderer(Image &img) : target(img)
    {
        target.clear(0, 0, 0); // black
    }

    ~SoftwareRenderer()
    {
        // stbtt_FreeBitmap(bitmap, nullptr);
    }

    void clear()
    {
        target.clear();
    }

    void drawRect(int x0, int y0, int w, int h, uint8_t r, uint8_t g, uint8_t b)
    {
        for (int y = y0; y < y0 + h; ++y)
            for (int x = x0; x < x0 + w; ++x)
                target.setPixel(x, y, r, g, b);
    }

    void savePNG(const std::string &filename)
    {
        stbi_write_png(filename.c_str(), target.width, target.height, 3, target.pixels.data(), target.width * 3);
    }

    bool loadFont(const std::string &fontPath)
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
        std::vector<unsigned char> fontBuffer(size);
        file.read((char *)fontBuffer.data(), size);

        if (!stbtt_InitFont(&font, fontBuffer.data(), 0))
        {
            std::cerr << "Error: stbtt_InitFont failed\n";
            return false;
        }
        return true;
    }

    void drawText(const std::string &text, int posX, int posY, float fontSize, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255)
    {
        float scale = stbtt_ScaleForMappingEmToPixels(&font, fontSize);

        // Get font metrics
        int ascent, descent, lineGap;
        stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
        int baseline = (int)(ascent * scale);
        std::cout << "scale: " << scale
                  << " ascent: " << ascent
                  << " descent: " << descent
                  << " lineGap: " << lineGap << std::endl;

        for (char c : text)
        {
            int width, height, xoff, yoff;
            unsigned char *bitmap = stbtt_GetCodepointBitmap(&font, 0, scale, c, &width, &height, &xoff, &yoff);

            std::cout << "Codepoint: " << c << " width: " << width << " height: " << height << " xoff: " << xoff << " yoff: " << yoff;

            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    uint8_t value = bitmap[y * width + x]; // 0..255
                    uint8_t r_ = (uint8_t)((float)r / 255.0) * (float)value;
                    uint8_t g_ = (uint8_t)((float)g / 255.0) * (float)value;
                    uint8_t b_ = (uint8_t)((float)b / 255.0) * (float)value;
                    // target.setPixel(x + posX, y + posY, r_, g_, b_);
                    target.setPixel(x + posX, y + posY, value, value, value);
                }
            }

            int advanceWidth, leftSideBearing;
            stbtt_GetCodepointHMetrics(&font, c, &advanceWidth, &leftSideBearing);
            std::cout << " advanceWidth: " << (float)advanceWidth * scale
                      << " leftSideBearing: " << (float)leftSideBearing * scale << std::endl;
            posX += advanceWidth * scale;
        }
    }

    // not needed anymore atm
    bool loadFontAsBitmap(const std::string &fontPath, float pixelHeight = 24.f)
    {
        std::ifstream file(fontPath, std::ios::binary);
        if (!file)
            return false;
        file.read(reinterpret_cast<char *>(fontBuffer), sizeof(fontBuffer));
        file.close();

        // bitmap size: 512 x 512
        int res = stbtt_BakeFontBitmap(fontBuffer, 0, pixelHeight, fontBitmap, 512, 512, 32, 96, cdata);

        if (res < 0)
        {
            std::cerr << "Font bake failed\n";
            return false;
        }

        return res >= 0;
    }

    // not needed anymore atm
    void drawTextFromFontBitmap(const std::string &text, int x, int y, uint8_t r, uint8_t g, uint8_t b, bool pixelPerfect = true)
    {
        float sx = 0, sy = 0;
        for (unsigned char c : text)
        {
            if (c < 32 || c >= 128)
                continue;
            stbtt_bakedchar *bchar = &cdata[c - 32];

            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(cdata, 512, 512, c - 32, &sx, &sy, &q, 1);

            // q.x0,q.y0,q.x1,q.y1 are float pixel coords in target img space
            int x0 = static_cast<int>(q.x0) + x;
            int y0 = static_cast<int>(q.y0) + y;
            int x1 = static_cast<int>(q.x1) + x;
            int y1 = static_cast<int>(q.y1) + y;

            // Draw glyph quad pixel-by-pixel, blending alpha
            for (int py = y0; py < y1; ++py)
            {
                for (int px = x0; px < x1; ++px)
                {
                    float u = q.s0 + (q.s1 - q.s0) * ((px - x0) / float(x1 - x0));
                    float v = q.t0 + (q.t1 - q.t0) * ((py - y0) / float(y1 - y0));
                    int src_x = int(u * 512);
                    int src_y = int(v * 512);
                    if (src_x < 0 || src_x >= 512 || src_y < 0 || src_y >= 512)
                        continue;
                    uint8_t alpha = fontBitmap[src_y * 512 + src_x];
                    if (alpha > 0)
                    {
                        if (pixelPerfect)
                            target.setPixel(px, py, r, g, b);
                        else
                            target.blendPixel(px, py, r, g, b, alpha);
                    }
                }
            }
        }
    }

private:
    stbtt_bakedchar cdata[96];           // ASCII 32..126
    unsigned char fontBuffer[1 << 20];   // 1 MB font file buffer
    unsigned char fontBitmap[512 * 512]; // grayscale glyph atlas
    Image &target;
    stbtt_fontinfo font;
};