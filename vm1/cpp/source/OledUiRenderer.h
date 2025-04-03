/*
 * Copyright (c) 2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include "imgui.h"
#include "MenuSystem.h"
#include "Registry.h"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengles2.h>
#include <SDL3/SDL_egl.h>

#include <queue>
#include <mutex>
#include <condition_variable>

struct ImageBuffer {
    ImageBuffer(int width, int height) {
        this->width = width;
        this->height = height;
        buffer = std::vector<unsigned char>(width * height * 4);
    }

    int width;
    int height;
    std::vector<unsigned char> buffer;
};

class OledUiRenderer
{
private:
    std::queue<ImageBuffer> m_imageQueue;
    std::mutex m_mutex;
    std::condition_variable m_cv;

    Registry &m_registry;
    int m_width = 0;
    int m_height = 0;

    ImGuiStyle m_oldStyle;
    ImGuiStyle m_style;
    ImFont *font_std;
    ImFont *font_big;

    GLuint m_fbo;
    GLuint m_fboTexture;

    MenuSystem m_menuSystem;

public:
    OledUiRenderer(Registry &registry, int width, int height);
    ~OledUiRenderer();

    GLuint texture();
    void initialize();
    void update();
    void queueCurrentImage();
    ImageBuffer popImage();
    void renderToRGB565(uint8_t *buffer, bool saveAsBmp = false);
    void renderToFramebuffer(bool saveAsPng = false);

private:
    void updateContent();
    void createFramebufferAndTexture();
    void createTheme();
    void setTheme();
    void resetTheme();
};
