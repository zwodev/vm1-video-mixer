#pragma once

#include "Shader.h"

#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengles2.h>
#include <SDL3/SDL_egl.h>

struct VertexWithTex
{
    float position[2];
    float texCoord[2];
};


class PlaneRenderer {

public: 
    PlaneRenderer();
    ~PlaneRenderer();

public:
    bool initialize();
    void update();

private:
    bool createVboFromVertices(const VertexWithTex* vertices, GLuint numVertices);
    void freeVbo();

private:
    GLuint m_vbo = 0;
    GLuint m_vao = 0;
    Shader m_shader;
};