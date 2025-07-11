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

StbRenderer::StbRenderer(int width, int height)
{
    img = std::make_unique<Image>(width, height);

    clear();

    if (!loadFont("fonts/ProggyClean.ttf"))
    {
        std::cerr << "Font load failed" << std::endl;
    }
    std::cout << "Renderer initialized" << std::endl;
}

StbRenderer::~StbRenderer()
{
    // stbtt_FreeBitmap(bitmap, nullptr);
}

void StbRenderer::clear()
{
    img->clear();
}

void StbRenderer::drawRect(int x0, int y0, int w, int h, c color)
{
    for (int y = y0; y < y0 + h; ++y)
        for (int x = x0; x < x0 + w; ++x)
            img->setPixel(x, y, color.r, color.g, color.b);
}

void StbRenderer::drawPng(const std::string &filename, int posX, int posY)
{
    int width, height, channels;
    unsigned char *data = stbi_load(
        filename.c_str(),
        &width,
        &height,
        &channels,
        0);

    if (data == nullptr)
    {
        std::cerr << "Failed to load PNG: " << filename << std::endl;
        return;
    }

    std::cout << "Loaded PNG: " << filename << std::endl;
    std::cout << "Size: " << width << " x " << height << std::endl;
    std::cout << "Channels: " << channels << std::endl;

    if (channels != 3)
        return;

    for (int x = 0; x < width; ++x)
    {
        for (int y = 0; y < height; ++y)
        {
            int pixel_index = (y * width + x) * channels;
            unsigned char r = data[pixel_index + 0];
            unsigned char g = data[pixel_index + 1];
            unsigned char b = data[pixel_index + 2];
            img->setPixel(x, y, r, g, b);
        }
    }
}

void StbRenderer::savePNG(const std::string &filename)
{
    stbi_write_png(filename.c_str(), img->width, img->height, 3, img->pixels.data(), img->width * 3);
}

bool StbRenderer::loadFont(const std::string &fontPath)
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

void StbRenderer::drawText(const std::string &text, int posX, int posY, float fontSize, c color)
{
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
        unsigned char *bitmap = stbtt_GetCodepointBitmap(&font, 0, scale, c, &width, &height, &xoff, &yoff);

        // std::cout << "Codepoint: " << c << " width: " << width << " height: " << height << " xoff: " << xoff << " yoff: " << yoff;

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                uint8_t value = bitmap[y * width + x]; // 0..255
                if (value == 0)
                    continue;
                uint8_t r_ = (uint8_t)((float)color.r / 255.0) * (float)value;
                uint8_t g_ = (uint8_t)((float)color.g / 255.0) * (float)value;
                uint8_t b_ = (uint8_t)((float)color.b / 255.0) * (float)value;
                img->setPixel(x + posX + xoff, y + posY + yoff + (fontSize / 2), r_, g_, b_);
            }
        }

        int advanceWidth, leftSideBearing;
        stbtt_GetCodepointHMetrics(&font, c, &advanceWidth, &leftSideBearing);
        // std::cout << " advanceWidth: " << (float)advanceWidth * scale
        //           << " leftSideBearing: " << (float)leftSideBearing * scale << std::endl;
        posX += advanceWidth * scale;
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
