/*
 * Copyright (c) 2023-2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 * 
 * Parts of this file have been taken from:
 * https://github.com/libsdl-org/SDL/blob/main/test/testffmpeg.c
 * 
 */


#include "VideoPlane.h"

const int NUM_PLAYERS = 2;

VideoPlane::VideoPlane()
{
    initialize();
}

VideoPlane::~VideoPlane()
{
    finalize();
}

void VideoPlane::initialize()
{
    for (int i = 0; i < NUM_PLAYERS; ++i) {
        VideoPlayer* player = new VideoPlayer();
        m_videoPlayers.push_back(player);
        m_yuvImages.push_back(YUVImage());
        m_startTimes.push_back(0);
    }
}

void VideoPlane::finalize()
{
    for (int i = 0; i < m_videoPlayers.size(); ++i) {
        VideoPlayer* player = m_videoPlayers[i];
        delete player;
    }
    m_videoPlayers.clear();
}

void VideoPlane::playAndFade(const std::string& fileName)
{
    if (m_isFading) return;

    int freePlayerIndex = 1;
    if (m_mixValue > 0.0f) freePlayerIndex = 0;

    VideoPlayer* videoPlayer = m_videoPlayers[freePlayerIndex];
    if (videoPlayer->open(fileName)) {
        m_startTimes[freePlayerIndex] = 0;
        videoPlayer->play();
        startFade();
    }
}

void VideoPlane::startFade() {
    m_isFading = true;

    m_fadeDir = 1.0f;
    if (m_mixValue > 0.0f) m_fadeDir = -1.0f;
}

void VideoPlane::updateFade(float deltaTime)
{
    if (!m_isFading) return;

    m_mixValue = m_mixValue + ((deltaTime / m_fadeTime) * m_fadeDir);
    if (m_mixValue <= 0.0f) {
        m_mixValue = 0.0f;
        m_isFading = false;
    } 
    else if (m_mixValue >= 1.0f) {
        m_mixValue = 1.0f;
        m_isFading = false;
    }
}

void VideoPlane::update(float deltaTime) {
    updateFade(deltaTime);
    updateVideoFrames(m_mixValue);
}

void VideoPlane::updateVideoFrames(float mixValue)
{
    EGLDisplay display = eglGetCurrentDisplay();

    // Wait for fence and delete it
    eglClientWaitSync(display, m_fence, EGL_SYNC_FLUSH_COMMANDS_BIT, EGL_FOREVER);
    eglDestroySync(display, m_fence);
    m_fence = EGL_NO_SYNC;

    // TODO: Can EGLImages be reused? It seems like the DRM-Buf FDs change very often.
    for (int i = 0; i < m_yuvImages.size(); ++i) {
        if (m_yuvImages[i].yImage != nullptr) {
            eglDestroyImage(display, m_yuvImages[i].yImage);
            m_yuvImages[i].yImage = nullptr;
        }
    }

    for (int i = 0; i < m_videoPlayers.size(); ++i) {
        VideoFrame frame;
        
        // Check PTS
        if (m_videoPlayers[i]->peekFrame(frame)) {
            if (!m_startTimes[i]) {
                m_startTimes[i] = SDL_GetTicks();
            }

            double pts = frame.pts;
            double now = (double)(SDL_GetTicks() - m_startTimes[i]) / 1000.0;
            
            // Do not pop and display frame when PTS is ahead 
            if (now < (pts - 0.001)) {
                //printf("Skipping frame because of PTS.\n");
                continue;
            } 
        }

        if (m_videoPlayers[i]->popFrame(frame)) {
            // Create EGL images here in the main thread
            // TODO: Support for multiple planes and images (see older version)
            if (frame.formats.size() > 0) {
                int j = 0;
                EGLAttrib img_attr[] = {
                    EGL_LINUX_DRM_FOURCC_EXT,      frame.formats[j],
                    EGL_WIDTH,                     frame.widths[j],
                    EGL_HEIGHT,                    frame.heights[j],
                    EGL_DMA_BUF_PLANE0_FD_EXT,     frame.fds[j],
                    EGL_DMA_BUF_PLANE0_OFFSET_EXT, frame.offsets[j],
                    EGL_DMA_BUF_PLANE0_PITCH_EXT,  frame.pitches[j],
                    EGL_NONE
                };
                
                EGLImage image = eglCreateImage(display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, img_attr);
                if (image != EGL_NO_IMAGE) {
                    m_yuvImages[i].yImage = image;
                }
            }
        }
    }

    // Render new frame
    m_planeRenderer.update(m_yuvImages, mixValue);

    // Create fence for current frame
    m_fence = eglCreateSync(display, EGL_SYNC_FENCE, NULL);
}

const std::vector<VideoPlayer*>& VideoPlane::players() const
{
    return m_videoPlayers;
}