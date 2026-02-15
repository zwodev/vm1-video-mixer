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

#include <SDL3/SDL_render.h> 
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengles2.h>
#include <SDL3/SDL_egl.h>

#include <linux/videodev2.h>

#include <vector>


    // virtual bool openFile(const std::string& fileName = std::string(), AudioStream* audioStream = nullptr) = 0;
    // virtual void update() = 0;
    // virtual void loadShaders() = 0;
    // virtual void run() = 0;

class ShaderPlayer : public MediaPlayer {
public: 
    ShaderPlayer();
    ~ShaderPlayer();

public:
    bool openFile(const std::string& fileName, AudioStream* audioStream = nullptr);
    void finalize();
    void update() override;
    void setCurrentTime(float time);

private:
    void loadShaders() override;
    void run() override;
    void render();

    void activateShader();
    void deactivateShader();

private:
    Shader m_customShader;
    float m_currentTime = 0.0f;

    int m_fd = -1;
    int m_bufferIndex = -1;
};