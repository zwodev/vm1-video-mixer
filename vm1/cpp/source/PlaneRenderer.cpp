/*
 * Copyright (c) 2023-2024 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include "GLHelper.h"
#include "PlaneRenderer.h"
#include <iostream>
#include <math.h>
#include <SDL3/SDL.h>
#include <GLES3/gl31.h>
using namespace std;

const int NUM_TEXTURES = 2;

PlaneRenderer::PlaneRenderer()
{
    initialize();
}

PlaneRenderer::~PlaneRenderer()
{ 

}

void PlaneRenderer::createVertexBuffers()
{   
    float quadVertices[] = {
        //  x,    y,    u,   v
        -1.0f, -1.0f,  0.0f, 0.0f, // bottom left
        1.0f, -1.0f,  1.0f, 0.0f, // bottom right
        1.0f,  1.0f,  1.0f, 1.0f, // top right

        -1.0f, -1.0f,  0.0f, 0.0f, // bottom left
        1.0f,  1.0f,  1.0f, 1.0f, // top right
        -1.0f,  1.0f,  0.0f, 1.0f  // top left
    };

    // Setup VAO, VBO, and attribute pointers for Position (vec2) and Texture coordinates (vec2)
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
   
    // Texture coordinates
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

bool PlaneRenderer::initialize()
{
    if (!m_shader.load("shaders/pass.vert", "shaders/screen.frag"))
        return false;

    createVertexBuffers();

    return true;
}

void PlaneRenderer::update(GLuint texture0, GLuint texture1, float mixValue)
{   
    m_shader.activate();
    glBindVertexArray(m_vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture0);
    m_shader.bindUniformLocation("inputTexture0", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture1);
    m_shader.bindUniformLocation("inputTexture1", 1);

    // Set mix value
    m_shader.setValue("mixValue", mixValue); 
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    m_shader.deactivate();

    // Unbind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
}


