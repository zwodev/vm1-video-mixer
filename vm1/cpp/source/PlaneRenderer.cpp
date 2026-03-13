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
#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/mat2x2.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>

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
    
    std::vector<glm::vec2> uvs = {glm::vec2(0.0f, 0.0f), 
                                  glm::vec2(1.0f, 0.0f), 
                                  glm::vec2(1.0f, 1.0f), 
                                  glm::vec2(0.0f, 1.0f)};

    
    unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };


    // Setup VAO, VBO, and attribute pointers for Position (vec2) and Texture coordinates (vec2)
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_posVbo);
    glGenBuffers(1, &m_uvVbo);
    glGenBuffers(1, &m_ibo);

    glBindVertexArray(m_vao);
    
    // Position
    glBindBuffer(GL_ARRAY_BUFFER, m_posVbo);
    glBufferData(GL_ARRAY_BUFFER, m_plane.size() * sizeof(glm::vec2), m_plane.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glEnableVertexAttribArray(0);
   
    // Texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER, m_uvVbo);
    glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), uvs.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glEnableVertexAttribArray(1);

    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    //printf("Size: %d\n", sizeof(m_plane));
    //printf("Size New: %d\n", m_plane.size() * sizeof(glm::vec2));
}

void PlaneRenderer::updateVertexBuffers(ScreenRotation rotation, PlaneSettings& planeSettings) {
    float degrees = float(int(rotation) * 90) + planeSettings.rotation;
    float angle = glm::radians(degrees);
    // printf("degrees: %f\n", degrees);
    float scale = planeSettings.scale;
    glm::vec2 scaleXY = planeSettings.scaleXY;
    glm::vec2 translation = planeSettings.translation;
    
    glm::mat2x2 screenRotationMatrix(cos(angle), -sin(angle), sin(angle),  cos(angle));
    glm::mat2x2 scaleMatrix(scale * scaleXY.x, 0.0f, 0.0f, scale * scaleXY.y);
    for (int i = 0; i < m_plane.size(); ++i) {
        // const glm::vec2& vertex = m_plane[i];
        // m_rotatedPlane[i] = screenRotationMatrix * (vertex + (planeSettings.coords[i] - m_plane[i]));
        glm::vec2 v = planeSettings.coords[i];
        v = scaleMatrix * v;
        v += translation;
        v = screenRotationMatrix * v;
        m_rotatedPlane[i] = v;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_posVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_rotatedPlane.size() * sizeof(glm::vec2), m_rotatedPlane.data());
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
    m_shader.setValue("opacity", planeSettings.opacity);
    for (auto& effect : effects) {
        for (auto& kv : effect.params) {
            const std::string& name = kv.first;
            auto& param = kv.second;
            if (std::holds_alternative<IntParameter>(param)) {
                auto& intParam = std::get<IntParameter>(param);
                std::string uniformName = effect.name + "_" + intParam.name;
                m_shader.setValue(uniformName.c_str(), intParam.value);
            } else if (std::holds_alternative<FloatParameter>(param)) {
                auto& floatParam = std::get<FloatParameter>(param);
                std::string uniformName = effect.name + "_" + floatParam.name;
                m_shader.setValue(uniformName.c_str(), floatParam.value);
            } 
        }
    }

    int isMultiplication = 0;
    switch (planeSettings.blendMode) {
        case PlaneSettings::BlendMode::BM_Alpha:
            glEnable(GL_BLEND); 
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case PlaneSettings::BlendMode::BM_Multiply:
            isMultiplication = 1;
            glEnable(GL_BLEND);
            glBlendFunc(GL_ZERO, GL_SRC_COLOR);
            break;
        default:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
    }
    m_shader.setValue("isMultiplication", isMultiplication);

    //glDrawArrays(GL_TRIANGLES, 0, 6);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glDisable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(0);
    m_shader.deactivate();

    // Unbind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
}


