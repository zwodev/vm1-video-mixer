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
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_DYNAMIC_DRAW);

    // Position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
   
    // Texture coordinates
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void PlaneRenderer::updateVertexBuffers(ScreenRotation rotation, PlaneSettings& planeSettings) {
    struct vec2 {float x, y;};
    vec2 bl, br, tr, tl;
    bl.x = planeSettings.coords.at(0).x;
    bl.y = planeSettings.coords.at(0).y;
    br.x = planeSettings.coords.at(1).x;
    br.y = planeSettings.coords.at(1).y;
    tr.x = planeSettings.coords.at(2).x;
    tr.y = planeSettings.coords.at(2).y;
    tl.x = planeSettings.coords.at(3).x;
    tl.y = planeSettings.coords.at(3).y;

    float quadVertices_0[] = {
        //  x,    y,    u,   v
        bl.x, bl.y,  0.0f, 0.0f, // bottom left
        br.x, br.y,  1.0f, 0.0f, // bottom right
        tr.x, tr.y,  1.0f, 1.0f, // top right

        bl.x, bl.y,  0.0f, 0.0f, // bottom left
        tr.x, tr.y,  1.0f, 1.0f, // top right
        tl.x, tl.y,  0.0f, 1.0f  // top left
    };
    float quadVertices_90[] = {
        //  x,    y,    u,   v
        bl.x, bl.y,  0.0f, 1.0f, // bottom left
        br.x, br.y,  0.0f, 0.0f, // bottom right
        tr.x, tr.y,  1.0f, 0.0f, // top right

        bl.x, bl.y,  0.0f, 1.0f, // bottom left
        tr.x, tr.y,  1.0f, 0.0f, // top right
        tl.x, tl.y,  1.0f, 1.0f  // top left
    };
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);  // Bind VBO directly (VAO bind optional but recommended for state)
    if(rotation == ScreenRotation::SR_Rotate_0) {
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quadVertices_0), quadVertices_0);
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quadVertices_90), quadVertices_90);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

bool PlaneRenderer::initialize()
{
    if (!m_shader.load("shaders/pass.vert", "shaders/plane_with_effects.frag"))
        return false;
    
    //if (!m_shader.load("shaders/pass.vert", "shaders/plane.frag"))
    //   return false;

    createVertexBuffers();

    return true;
}

void PlaneRenderer::update(GLuint texture0, GLuint texture1, float mixValue, PlaneSettings& planeSettings, ScreenRotation rotation)
{   
    std::vector<ShaderConfig>& effects = planeSettings.effects;
    updateVertexBuffers(rotation, planeSettings);

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
    for (auto& effect : effects) {
        for (auto& param : effect.params) {
            if (std::holds_alternative<IntParameter>(param)) {
                auto& intParam = std::get<IntParameter>(param);
                std::string uniformName = effect.name + "_" + intParam.name;
                m_shader.setValue(uniformName.c_str(), intParam.value);
            } else if (std::holds_alternative<FloatParameter>(param)) {
                auto& floatParam = std::get<FloatParameter>(param);
                std::string uniformName = effect.name + "_" + floatParam.name;
                m_shader.setValue(uniformName.c_str(), floatParam.value);
            } else if (std::holds_alternative<ColorParameter>(param)) {
                // add ColorParam
            }
        }
    }

    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    m_shader.deactivate();

    // Unbind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
}


