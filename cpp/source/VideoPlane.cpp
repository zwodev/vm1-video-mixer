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

// void VideoPlane::update()
// {
//     for (int i = 0; i < m_videoPlayers.size(); ++i) {
//         m_videoPlayers[i]->update();
//     }
// }

void VideoPlane::update(float mixValue) {
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
            // if (frame.formats.size() > 1) {
            //     int j = 1;
            //     EGLAttrib img_attr[] = {
            //         EGL_LINUX_DRM_FOURCC_EXT,      frame.formats[j],
            //         EGL_WIDTH,                     frame.widths[j],
            //         EGL_HEIGHT,                    frame.heights[j],
            //         EGL_DMA_BUF_PLANE0_FD_EXT,     frame.fds[j],
            //         EGL_DMA_BUF_PLANE0_OFFSET_EXT, frame.offsets[j],
            //         EGL_DMA_BUF_PLANE0_PITCH_EXT,  frame.pitches[j],
            //         EGL_NONE
            //     };
                
            //     EGLImage image = eglCreateImage(display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, img_attr);
            //     if (image != EGL_NO_IMAGE) {
            //         m_yuvImages[i].uvImage = image;
            //     }
            // }
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