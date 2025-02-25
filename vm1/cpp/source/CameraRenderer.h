/*
 * Copyright (c) 2025 Nils Zweiling
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

#include <linux/videodev2.h>

#include <vector>

struct Buffer {
    size_t length;
    int fd;
    EGLImage image;
};

class CameraRenderer {

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

public: 
    CameraRenderer();
    ~CameraRenderer();

public:
    bool start();
    void update();

private:
    bool setFormat(int fd);
    bool initBuffers(int fd);
    bool queueBuffer(int fd, int index);
    int dequeueBuffer(int fd);
    bool initialize();
    void createGeometryBuffers();
    bool createVbo();
    bool createIbo();
    void freeVbo();

private:
    int m_fd = -1;
    v4l2_format m_fmt;
    std::vector<Buffer> m_buffers;
    bool m_isRunning = false;
    std::vector<EGLImage> m_images;
    std::vector<VertexWithTex> m_vertices;
    std::vector<GLuint> m_indices;
    GLuint m_rgbTexture;
    GLuint m_ibo = 0;
    GLuint m_vbo = 0;
    GLuint m_vao = 0;
    Shader m_shader;
};