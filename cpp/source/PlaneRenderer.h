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
    VertexWithTex(float p_x, float p_y, float p_u, float p_v, float p_offset) {
        x = p_x * 2.0f - 1.0f;
        y = p_y * 2.0f - 1.0f;
        u = p_u;
        v = p_v;
        offset = p_offset;
    }

    float x;
    float y;
    float u;
    float v;
    float offset;

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
    void update(std::vector<YUVImage> yuvImages, float mixValue);

private:
    void createGeometryBuffers();
    void runComputeShader();
    bool createVbo();
    bool createIbo();
    void freeVbo();

private:
    GLuint m_coordTexture;
    std::vector<VertexWithTex> m_vertices;
    std::vector<GLuint> m_indices;
    std::vector<GLuint> m_yuvTextures;
    std::vector<GLuint> m_compTextures;
    std::vector<GLuint> m_inputTextures;
    GLuint m_compTexture;
    GLuint m_frameBuffer;
    GLuint m_ibo = 0;
    GLuint m_vbo = 0;
    GLuint m_vao = 0;
    Shader m_shader;
    Shader m_compShader;
    Shader m_mixShader;
};