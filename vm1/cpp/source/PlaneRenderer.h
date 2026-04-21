/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include "Shader.h"
#include "ShaderConfig.h"
#include "ScreenOptions.h"
#include "Registry.h"

#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengles2.h>
#include <SDL3/SDL_egl.h>

#include <vector>

class PlaneRenderer {

public:
    struct InternalShaderParams {
        GLuint texture0 = 0;
        GLuint texture1 = 0;
        float mixValue = 0.0f;
        float iTime = 0.0f;
    };

public: 
    PlaneRenderer();
    ~PlaneRenderer();

public:
    bool initialize();
    const ShaderConfig& shaderConfig();
    bool loadShader(const std::string& extFilename = "");
    //void update(GLuint texture0, GLuint texture1, float mixValue, PlaneSettings& planeSettings, ScreenRotation rotation);
    void update(PlaneSettings& planeSettings, ScreenRotation rotation, InternalShaderParams internalShaderParams);
    

private:
    void createVertexBuffers();
    void updateVertexBuffers(ScreenRotation rotation, PlaneSettings& PlaneSettings);

private:
    GLuint m_vao; 
    GLuint m_posVbo;
    GLuint m_uvVbo;
    GLuint m_ibo;
    Shader m_shader;

    std::vector<glm::vec2> m_plane = { glm::vec2(-1.0f, -1.0f), 
                                       glm::vec2(1.0f, -1.0f), 
                                       glm::vec2(1.0f, 1.0f), 
                                       glm::vec2(-1.0f, 1.0f) };

    std::vector<glm::vec2> m_rotatedPlane = m_plane;
};