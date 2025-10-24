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

#include <SDL3/SDL_render.h> 
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengles2.h>
#include <SDL3/SDL_egl.h>

#include <linux/videodev2.h>

#include <vector>

class CameraPlayer : public MediaPlayer {
public: 
    CameraPlayer();
    ~CameraPlayer();

public:
    bool openFile(const std::string& fileName, AudioStream* audioStream = nullptr);
    void close() override;
    void finalize();
    void lockBuffer();
    Buffer* getBuffer();
    void unlockBuffer();
    void update() override;

    // TODO: Needs to be determined by HDMI port oder dev (/dev/video0)
    int getPort() { return 0; } 

private:
    void loadShaders() override;
    void run() override;
    void render();
    
    bool setFormat(int fd);
    bool initBuffers(int fd);
    bool queueBuffer(int fd, int index);
    int dequeueBuffer(int fd);

private:
    int m_fd = -1;
    int m_bufferIndex = -1;
    v4l2_format m_fmt;
    std::vector<Buffer> m_buffers;
};