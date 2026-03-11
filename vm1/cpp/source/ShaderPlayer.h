/*
 * Copyright (c) 2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */


#pragma once

#include "MediaPlayer.h"
#include "Buffer.h"
#include "Shader.h"
#include "CaptureType.h"
#include "ShaderConfig.h"

#include <SDL3/SDL_render.h> 
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengles2.h>
#include <SDL3/SDL_egl.h>

#include <linux/videodev2.h>

#include <vector>

class ShaderPlayer : public MediaPlayer {
public: 
    ShaderPlayer();
    ~ShaderPlayer();

public:
    bool openFile(const std::string& fileName, AudioStream* audioStream = nullptr);
    void close() override;
    void finalize();
    void update() override;
    const ShaderConfig& shaderConfig();
    void setShaderUniforms(const ShaderConfig& shaderConfig);
    void setCurrentTime(float time);
    void setAnalogValue(float value);

private:
    void loadShaders() override;
    void run() override;
    void render();

    void activateShader();
    void deactivateShader();

private:
    //Shader m_customShader;
    ShaderConfig m_shaderConfig;
    float m_currentTime = 0.0f;
    float m_analogValue = 0.0f;
    bool m_isShaderReady = false;

    int m_fd = -1;
    int m_bufferIndex = -1;
};