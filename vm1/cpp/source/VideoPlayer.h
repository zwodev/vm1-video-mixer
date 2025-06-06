/*
 * Copyright (c) 2023-2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */


#pragma once

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

#include "PlaneRenderer.h"

static SDL_PixelFormat getTextureFormat(enum AVPixelFormat format);
static bool isSupportedPixelFormat(enum AVPixelFormat format);
static enum AVPixelFormat getSupportedPixelFormat(AVCodecContext *s, const enum AVPixelFormat *pix_fmts);

struct VideoFrame {
    std::vector<EGLImage> images;
    bool isFirstFrame = false;
    double pts = 0.0;
    
    // Add these fields for DRM frame info
    std::vector<uint32_t> formats;
    std::vector<int> widths;
    std::vector<int> heights;
    std::vector<int> fds;
    std::vector<uint32_t> offsets;
    std::vector<uint32_t> pitches;
};

class VideoPlayer {
public:
    VideoPlayer();
    ~VideoPlayer();

    bool open(std::string fileName, bool useH264 = false);
    void play();
    void close();
    bool isPlaying() const { return m_isRunning; }
    void setLooping(bool looping);
    bool popFrame(VideoFrame& frame);
    bool peekFrame(VideoFrame& frame);

private:
    AVCodecContext* openVideoStream();
    AVCodecContext* openAudioStream();
    bool getTextureForDRMFrame(AVFrame *frame, VideoFrame &dstFram);
    void decodingThread();
    void cleanupResources();
    void pushFrame(VideoFrame& frame);
    void clearFrames();
    

private:
    // FFMpeg
    double m_firstPts = -1.0;
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

    // Threading & Synchronization
    std::thread m_decoderThread;
    std::mutex m_frameMutex;
    std::condition_variable m_frameCV;
    std::queue<EGLSyncKHR> m_fences;
    
    // Frames
    static constexpr size_t MAX_QUEUE_SIZE = 3;
    VideoFrame m_currentFrame;
    std::queue<VideoFrame> m_frameQueue;
    std::vector<VideoFrame> m_framesToDelete;

    // State
    std::atomic<bool> m_isRunning = false;
    std::atomic<bool> m_isLooping = false;
    std::atomic<bool> m_isFlushing = false;
};