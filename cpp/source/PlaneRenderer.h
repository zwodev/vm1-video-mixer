/*
 * Copyright (c) 2023-2024 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */


#pragma once

#include "Shader.h"

#include <SDL3/SDL_render.h> 
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengles2.h>
#include <SDL3/SDL_egl.h>

#include <vector>

struct vec2
{
    vec2(float p_x, float p_y) {
        x = p_x;
        y = p_y;
    }

    float x;
    float y;
};

struct VertexWithTex
{
    float position[2];
    float texCoord[2];
};

struct YUVImage {
    EGLImage yImage = nullptr;
    EGLImage uvImage = nullptr;
};

class PlaneRenderer {

public: 
    PlaneRenderer();
    ~PlaneRenderer();

public:
    bool initialize();
    void update(EGLImage image);
    void update(std::vector<YUVImage> yuvImages, float mixValue);
    //void update(SDL_Texture* texture);

private:
    void createAndFillTexture();
    bool createVboFromVertices(const VertexWithTex* vertices, GLuint numVertices);
    void freeVbo();

private:
    GLuint m_coordTexture;
    std::vector<GLuint> m_yuvTextures;
    GLuint m_vbo = 0;
    GLuint m_vao = 0;
    Shader m_shader;
};