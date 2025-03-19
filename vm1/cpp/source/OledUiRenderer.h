/*
 * Copyright (c) 2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include "imgui.h"
#include "MenuTest.h"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengles2.h>
#include <SDL3/SDL_egl.h>

class OledUiRenderer
{
private:
    int m_width = 0;
    int m_height = 0;

    ImGuiStyle m_oldStyle;
    ImGuiStyle m_style;

    GLuint m_fbo;
    GLuint m_fboTexture;

    bool isUpdated;

    MenuSystem menuSystem;

public:
    OledUiRenderer(int width, int height);
    ~OledUiRenderer();

    GLuint texture();
    void initialize();
    void update();
    void renderToRGB565(uint8_t *buffer);
    bool hasUpdate();

private:
    void updateContent();
    void createFramebufferAndTexture();
    void renderToFramebuffer(bool saveAsPng);
    void createTheme();
    void setTheme();
    void resetTheme();
};
