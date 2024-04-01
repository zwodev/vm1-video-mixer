/*
 * Copyright (c) 2023-2024 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/LICENSE
 * for full license details.
 */


#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_egl.h>

#include <iostream>
#include <string>
#include <vector>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include <libavutil/hwcontext_drm.h>
}

#include "PlaneRenderer.h"

static void setYUVConversionMode(AVFrame *frame);
static Uint32 getTextureFormat(enum AVPixelFormat format);
static bool isSupportedPixelFormat(enum AVPixelFormat format);
static enum AVPixelFormat getSupportedPixelFormat(AVCodecContext *s, const enum AVPixelFormat *pix_fmts);

class VideoPlayer {
public:
    VideoPlayer();
    ~VideoPlayer();

public: 
    bool open(std::string fileName, bool useH264 = false);
    void update();

private:
    AVCodecContext* openVideoStream();
    AVCodecContext* openAudioStream();
    void handleVideoFrame(AVFrame *frame, double pts);
    void displayVideoTexture(AVFrame *frame);
    bool getTextureForFrame(AVFrame *frame);
    bool getTextureForMemoryFrame(AVFrame *frame);
    bool getTextureForVAAPIFrame(AVFrame *frame);
    bool getTextureForDRMFrame(AVFrame *frame);

private:
    std::vector<EGLImage> m_images;
    PlaneRenderer m_planeRenderer;
    Uint64 m_videoStart = 0;
    double m_firstPts = -1.0;
    SDL_Texture* m_videoTexture = nullptr;
    bool m_hasEglCreateImage = false;
    bool m_flushing = false;
    SDL_AudioStream* m_audio;
    AVFormatContext* m_formatContext = nullptr;
    const AVCodec* m_audioCodec = nullptr;
    const AVCodec* m_videoCodec = nullptr;
    AVCodecContext* m_audioContext = nullptr;
    AVCodecContext* m_videoContext = nullptr;
    AVPacket* m_packet = nullptr;
    AVFrame*  m_frame = nullptr;
    int m_videoStream = -1;
    int m_audioStream = -1;
};