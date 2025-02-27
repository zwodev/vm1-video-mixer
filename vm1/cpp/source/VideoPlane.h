/*
 * Copyright (c) 2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */


#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_egl.h>

#include <string>
#include <vector>

#include "VideoPlayer.h"
#include "PlaneRenderer.h"

class VideoPlane {
public:
    VideoPlane();
    ~VideoPlane();

public:
    const std::vector<VideoPlayer*>& players() const;
    void playAndFade(const std::string& fileName);
    void update(float mixValue);

private:
    void initialize();
    void finalize();
    void startFade();
    void updateFade(float deltaTime);
    void updateVideoFrames(float mixValue);
    
private:
    bool m_isFading = false;
    float m_fadeTime = 2.0f;
    float m_fadeDir = 1.0f;
    float m_mixValue = 0.0f;

    std::vector<Uint64> m_startTimes;
    std::vector<YUVImage> m_yuvImages;
    PlaneRenderer m_planeRenderer;
    std::vector<VideoPlayer*> m_videoPlayers;
    EGLSyncKHR m_fence = EGL_NO_SYNC;
};