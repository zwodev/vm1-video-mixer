/*
 * Copyright (c) 2023-2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */


#pragma once

#include "source/MediaPlayer.h"
#include "source/Shader.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_egl.h>

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <condition_variable>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include <libavutil/hwcontext_drm.h>
}


static SDL_PixelFormat getTextureFormat(enum AVPixelFormat format);
static bool isSupportedPixelFormat(enum AVPixelFormat format);
static enum AVPixelFormat getSupportedPixelFormat(AVCodecContext* s, const enum AVPixelFormat* pix_fmts);


class VideoPlayer : public MediaPlayer {
public:
    VideoPlayer();
    ~VideoPlayer();

    bool openFile(const std::string& fileName, AudioStream* audioStream = nullptr) override;
    void close() override;
    void setLooping(bool looping);
    void update() override;
    
private:
    void reset() override;
    void loadShaders() override;
    void run() override;
    void render();
    

    AVCodecContext* openVideoStream();
    AVCodecContext* openAudioStream();
    void handleAudioFrame(AVFrame* frame);
    bool getTextureForDRMFrame(AVFrame* frame, VideoFrame& dstFrame);


private:
    // FFMpeg
    Uint64 m_startTime = 0;
    double m_firstPts = -1.0;
    double m_firstAudioPts = -1.0;
    AVFormatContext* m_formatContext = nullptr;
    //AVDictionary* m_options = nullptr;
    const AVCodec* m_audioCodec = nullptr;
    const AVCodec* m_videoCodec = nullptr;
    AVCodecContext* m_audioContext = nullptr;
    AVCodecContext* m_videoContext = nullptr;
    AVPacket* m_packet = nullptr;
    AVFrame*  m_frame = nullptr;
    int m_videoStream = -1;
    int m_audioStream = -1;

    // State
    std::atomic<bool> m_isLooping = false;
    std::atomic<bool> m_isFlushing = false;
};