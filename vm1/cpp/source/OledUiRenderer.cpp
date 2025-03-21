/*
 * Copyright (c) 2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include "OledUiRenderer.h"

#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <SDL3_image/SDL_image.h>

#include <vector>
#include <iostream>

#include <fstream>
#include <cstdint>

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

OledUiRenderer::OledUiRenderer(Registry &registry, int width, int height) : m_registry(registry),
                                                                            m_menuSystem(registry),
                                                                            m_width(width),
                                                                            m_height(height)
{
}

OledUiRenderer::~OledUiRenderer()
{
    //
}

void OledUiRenderer::initialize()
{
    ImGuiStyle &style = ImGui::GetStyle();
    m_style = style;
    m_oldStyle = style;
    createTheme();

    ImGuiIO &fbo_io = ImGui::GetIO();
    font_std = fbo_io.Fonts->AddFontFromFileTTF("subprojects/imgui/imgui/misc/fonts/ProggyClean.ttf", 13.0f);
    font_big = fbo_io.Fonts->AddFontFromFileTTF("subprojects/imgui/imgui/misc/fonts/ProggyClean.ttf", 26.0f);
    fbo_io.FontDefault = font_std;
    fbo_io.Fonts->Build();

    createFramebufferAndTexture();
}

GLuint OledUiRenderer::texture()
{
    return OledUiRenderer::m_fboTexture;
}

void OledUiRenderer::createFramebufferAndTexture()
{
    glGenFramebuffers(1, &m_fbo);
    glGenTextures(1, &m_fboTexture);

    glBindTexture(GL_TEXTURE_2D, m_fboTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fboTexture, 0);

    // debug: get size of the framebuffer
    GLint internalFormat;

    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
    printf("Pixel Format: %d\n", internalFormat);
}

void OledUiRenderer::update()
{
    // Set OLED theme
    setTheme();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(m_width, m_height));

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    ImGui::Begin("Framebuffer Window", nullptr, window_flags);

    updateContent();

    ImGui::End();

    renderToFramebuffer(false);

    // Reset to old theme
    resetTheme();
}

// PRIVATE

// USE THIS TO CHANGE THE UI
void OledUiRenderer::updateContent()
{
    // Render the actual UI (flags: no title, borderless, etc)
    m_menuSystem.render();
}

void OledUiRenderer::renderToFramebuffer(bool saveAsPng)
{
    // Bind the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);

    // Render ImGui
    // glDisable(GL_MULTISAMPLE);
    ImGui::Render();
    ImDrawData *drawData = ImGui::GetDrawData();
    drawData->DisplaySize = ImVec2(m_width, m_height);
    ImGui_ImplOpenGL3_RenderDrawData(drawData);

    if (saveAsPng)
    {
        // Read pixels from the FBO
        std::vector<unsigned char> pixels(m_width * m_height * 4);
        glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        // Save to PNG
        // Create an SDL surface from the pixel data
        SDL_Surface *surface = SDL_CreateSurfaceFrom(m_width, m_height, SDL_PIXELFORMAT_RGBA32, pixels.data(), m_width * 4);

        if (surface)
        {
            SDL_FlipSurface(surface, SDL_FLIP_VERTICAL);
            // Save the SDL surface as a PNG using IMG_SavePNG
            if (IMG_SavePNG(surface, "output.png"))
            {
                printf("Image saved successfully\n");
            }
            else
            {
                printf("Failed to save image: %s\n", SDL_GetError());
            }

            SDL_DestroySurface(surface);
        }
        else
        {
            printf("Failed to create surface: %s\n", SDL_GetError());
        }
    }

    // Unbind the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OledUiRenderer::renderToRGB565(uint8_t *buffer, bool saveAsBmp)
{
    // Bind the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    // glViewport(0, 0, m_width, m_height);

    // Read pixels from the FBO
    std::vector<unsigned char> pixels(m_width * m_height * 4);
    glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // Now you can process the data
    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            // Calculate the flipped y index (flip vertically)
            int flippedY = m_height - 1 - y;

            // Fetch RGBA values from the texture buffer (no horizontal flip)
            uint8_t r = pixels[(flippedY * m_width + x) * 4 + 0];
            uint8_t g = pixels[(flippedY * m_width + x) * 4 + 1];
            uint8_t b = pixels[(flippedY * m_width + x) * 4 + 2];

            // Convert to RGB565
            uint16_t rgb565 = (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3);

            // Store the 16-bit RGB565 into the buffer as two bytes
            buffer[(y * m_width + x) * 2 + 0] = (uint8_t)(rgb565 >> 8);   // High byte (most significant byte)
            buffer[(y * m_width + x) * 2 + 1] = (uint8_t)(rgb565 & 0xFF); // Low byte (least significant byte)
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (saveAsBmp)
    {
        std::string filename = "output.bmp";
        // Ensure 4-byte row alignment
        int rowSize = ((m_width * 2 + 3) / 4) * 4;
        int dataSize = rowSize * m_height;
        int fileSize = sizeof(BMPFileHeader) + sizeof(BMPInfoHeaderV3) + dataSize;

        BMPFileHeader fileHeader;
        fileHeader.bfSize = fileSize;

        BMPInfoHeaderV3 infoHeader;
        infoHeader.biWidth = m_width;
        infoHeader.biHeight = -m_height; // Top-down DIB
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
        for (int y = 0; y < m_height; ++y)
        {
            memcpy(rowBuffer.data(), &buffer[y * m_width * 2], m_width * 2);
            file.write(reinterpret_cast<const char *>(rowBuffer.data()), rowSize);
        }

        file.close();
        printf("Saved BMP: %s\n", filename.c_str());
    }
}

void OledUiRenderer::createTheme()
{
    // TODO: Modify theme here for use with OLED
    m_style.GrabRounding = 4.0f;
    m_style.TabRounding = 4.0f;
    m_style.ChildRounding = 4.0f;
    m_style.FrameRounding = 4.0f;
    m_style.PopupRounding = 4.0f;
    // m_style.WindowRounding = 4.0f;
    m_style.ScrollbarRounding = 4.0f;

    m_style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    m_style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    // m_style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    // m_style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    // m_style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    m_style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    m_style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

    // m_style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    m_style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    // m_style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
    m_style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

    m_style.AntiAliasedFill = false;
    m_style.AntiAliasedLines = false;

    m_style.ScrollbarRounding = 0.0f;
    m_style.ScrollbarSize = 1.0f;
    m_style.FrameBorderSize = 0.0f;
    m_style.WindowBorderSize = 0.0f;

    m_style.CellPadding = ImVec2(0.0f, 0.0f);
    m_style.FramePadding = ImVec2(0.0f, 0.0f);
    m_style.WindowPadding = ImVec2(0.0f, 0.0f);
    m_style.DisplayWindowPadding = ImVec2(0.0f, 0.0f);
    m_style.SeparatorTextPadding = ImVec2(0.0f, 0.0f);
    m_style.DisplaySafeAreaPadding = ImVec2(0.0f, 0.0f);
    // ...

    // EXAMPLE COLOR PROPERTIES:
    // ImVec4* colors = style->Colors;
    // colors[ImGuiCol_Text] = ColorConvertU32ToFloat4(Spectrum::GRAY800); // text on hovered controls is gray900
    // colors[ImGuiCol_TextDisabled] = ColorConvertU32ToFloat4(Spectrum::GRAY500);
    // colors[ImGuiCol_WindowBg] = ColorConvertU32ToFloat4(Spectrum::GRAY100);
    // colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    // colors[ImGuiCol_PopupBg] = ColorConvertU32ToFloat4(Spectrum::GRAY50); // not sure about this. Note: applies to tooltips too.
    // colors[ImGuiCol_Border] = ColorConvertU32ToFloat4(Spectrum::GRAY300);
    // colors[ImGuiCol_BorderShadow] = ColorConvertU32ToFloat4(Spectrum::Static::NONE); // We don't want shadows. Ever.
    // colors[ImGuiCol_FrameBg] = ColorConvertU32ToFloat4(Spectrum::GRAY75); // this isnt right, spectrum does not do this, but it's a good fallback
    // colors[ImGuiCol_FrameBgHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY50);
    // colors[ImGuiCol_FrameBgActive] = ColorConvertU32ToFloat4(Spectrum::GRAY200);
    // colors[ImGuiCol_TitleBg] = ColorConvertU32ToFloat4(Spectrum::GRAY300); // those titlebar values are totally made up, spectrum does not have this.
    // colors[ImGuiCol_TitleBgActive] = ColorConvertU32ToFloat4(Spectrum::GRAY200);
    // colors[ImGuiCol_TitleBgCollapsed] = ColorConvertU32ToFloat4(Spectrum::GRAY400);
    // colors[ImGuiCol_MenuBarBg] = ColorConvertU32ToFloat4(Spectrum::GRAY100);
    // colors[ImGuiCol_ScrollbarBg] = ColorConvertU32ToFloat4(Spectrum::GRAY100); // same as regular background
    // colors[ImGuiCol_ScrollbarGrab] = ColorConvertU32ToFloat4(Spectrum::GRAY400);
    // colors[ImGuiCol_ScrollbarGrabHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY600);
    // colors[ImGuiCol_ScrollbarGrabActive] = ColorConvertU32ToFloat4(Spectrum::GRAY700);
    // colors[ImGuiCol_CheckMark] = ColorConvertU32ToFloat4(Spectrum::BLUE500);
    // colors[ImGuiCol_SliderGrab] = ColorConvertU32ToFloat4(Spectrum::GRAY700);
    // colors[ImGuiCol_SliderGrabActive] = ColorConvertU32ToFloat4(Spectrum::GRAY800);
    // colors[ImGuiCol_Button] = ColorConvertU32ToFloat4(Spectrum::GRAY75); // match default button to Spectrum's 'Action Button'.
    // colors[ImGuiCol_ButtonHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY50);
    // colors[ImGuiCol_ButtonActive] = ColorConvertU32ToFloat4(Spectrum::GRAY200);
    // colors[ImGuiCol_Header] = ColorConvertU32ToFloat4(Spectrum::BLUE400);
    // colors[ImGuiCol_HeaderHovered] = ColorConvertU32ToFloat4(Spectrum::BLUE500);
    // colors[ImGuiCol_HeaderActive] = ColorConvertU32ToFloat4(Spectrum::BLUE600);
    // colors[ImGuiCol_Separator] = ColorConvertU32ToFloat4(Spectrum::GRAY400);
    // colors[ImGuiCol_SeparatorHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY600);
    // colors[ImGuiCol_SeparatorActive] = ColorConvertU32ToFloat4(Spectrum::GRAY700);
    // colors[ImGuiCol_ResizeGrip] = ColorConvertU32ToFloat4(Spectrum::GRAY400);
    // colors[ImGuiCol_ResizeGripHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY600);
    // colors[ImGuiCol_ResizeGripActive] = ColorConvertU32ToFloat4(Spectrum::GRAY700);
    // colors[ImGuiCol_PlotLines] = ColorConvertU32ToFloat4(Spectrum::BLUE400);
    // colors[ImGuiCol_PlotLinesHovered] = ColorConvertU32ToFloat4(Spectrum::BLUE600);
    // colors[ImGuiCol_PlotHistogram] = ColorConvertU32ToFloat4(Spectrum::BLUE400);
    // colors[ImGuiCol_PlotHistogramHovered] = ColorConvertU32ToFloat4(Spectrum::BLUE600);
    // colors[ImGuiCol_TextSelectedBg] = ColorConvertU32ToFloat4((Spectrum::BLUE400 & 0x00FFFFFF) | 0x33000000);
    // colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    // colors[ImGuiCol_NavHighlight] = ColorConvertU32ToFloat4((Spectrum::GRAY900 & 0x00FFFFFF) | 0x0A000000);
    // colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    // colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    // colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

void OledUiRenderer::setTheme()
{
    ImGuiStyle &style = ImGui::GetStyle();
    style = m_style;
}

void OledUiRenderer::resetTheme()
{
    ImGuiStyle &style = ImGui::GetStyle();
    style = m_oldStyle;
}
