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

const int NUM_VIDEO_PLAYERS = 2;
const int NUM_CAMERA_PLAYERS = 1;

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
    // Create video players
    for (int i = 0; i < NUM_VIDEO_PLAYERS; ++i) {
        VideoPlayer* player = new VideoPlayer();
        m_videoPlayers.push_back(player);
        m_yuvImages.push_back(YUVImage());
        m_startTimes.push_back(0);
    }

    // // Create camera players
    // for (int i = 0; i < NUM_CAMERA_PLAYERS; ++i) {
    //     CameraPlayer* player = new CameraPlayer();
    //     m_cameraPlayers.push_back(player);
    //     m_yuyvImages.push_back(YUVImage());
    // }
}

void VideoPlane::finalize()
{
    for (int i = 0; i < m_videoPlayers.size(); ++i) {
        VideoPlayer* player = m_videoPlayers[i];
        delete player;
    }
    m_videoPlayers.clear();
}

void VideoPlane::addCameraPlayer(CameraPlayer* cameraPlayer)
{
    m_cameraPlayers.push_back(cameraPlayer);
    m_yuyvImages.push_back(YUVImage());
}

float VideoPlane::fadeTime() const
{
    return m_fadeTime;
}

void VideoPlane::setFadeTime(int fadeTime)
{
    m_fadeTime = fadeTime;
}

void VideoPlane::playAndFade(const std::string& fileName)
{
    if (m_isVideoFading || m_isCameraFading) return;

    if (fileName == "../videos/hdmi0") {
        CameraPlayer* cameraPlayer = m_cameraPlayers[0];
        if (cameraPlayer->start()) {
            if (m_cameraMixValue < 1) startCameraFade();
        }
    }
    else {
        int freePlayerIndex = 1;
        if (m_videoMixValue > 0.0f) freePlayerIndex = 0;

        VideoPlayer* videoPlayer = m_videoPlayers[freePlayerIndex];
        if (videoPlayer->open(fileName)) {
            m_startTimes[freePlayerIndex] = 0;
            videoPlayer->play();
            if (m_cameraMixValue > 0) {
                startCameraFade();
                m_videoMixValue = freePlayerIndex;
            }
            else {
                startVideoFade();
            }
        }
    }
}

void VideoPlane::startVideoFade() {
    m_isVideoFading = true;
    m_fadeDir = 1.0f;
    if (m_videoMixValue > 0.0f) m_fadeDir = -1.0f;
}

void VideoPlane::startCameraFade() {
    m_isCameraFading = true;
    m_fadeDir = 1.0f;
    if (m_cameraMixValue > 0.0f) m_fadeDir = -1.0f;
}

void VideoPlane::updateFade(float deltaTime, float& mixValue, bool& isFading)
{
    mixValue = mixValue + ((deltaTime / m_fadeTime) * m_fadeDir);
    if (mixValue <= 0.0f) {
        mixValue = 0.0f;
        isFading = false;
    } 
    else if (mixValue >= 1.0f) {
        mixValue = 1.0f;
        isFading = false;
    }
}

void VideoPlane::update(float deltaTime) {
    if (m_isVideoFading) {
        updateFade(deltaTime, m_videoMixValue, m_isVideoFading);
        printf ("Video Mix: %f, Camera Mix: %f\n", m_videoMixValue, m_cameraMixValue);
    }
    else if (m_isCameraFading) {
        updateFade(deltaTime, m_cameraMixValue, m_isCameraFading);
        printf ("Video Mix: %f, Camera Mix: %f\n", m_videoMixValue, m_cameraMixValue);
    }
        
    updateVideoFrames(m_videoMixValue, m_cameraMixValue);
}

void VideoPlane::updateVideoFrames(float videoMixValue, float cameraMixValue)
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

    // Lock and fetch camera buffers
    for (int i = 0; i < m_cameraPlayers.size(); ++i) {
        Buffer* buffer = m_cameraPlayers[i]->getBuffer();
        if (buffer) {
            m_yuyvImages[i].yImage = buffer->image;
        }
    }

    // Render new frame
    m_planeRenderer.update(m_yuvImages, m_yuyvImages, videoMixValue, cameraMixValue);


    // Create fence for current frame
    m_fence = eglCreateSync(display, EGL_SYNC_FENCE, NULL);
}

const std::vector<VideoPlayer*>& VideoPlane::players() const
{
    return m_videoPlayers;
}