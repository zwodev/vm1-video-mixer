#pragma once

#include <SDL3/SDL.h>
#include <iostream>
#include <string>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include <libavutil/hwcontext_drm.h>

static Uint32 getTextureFormat(enum AVPixelFormat format);
static bool isSupportedPixelFormat(enum AVPixelFormat format);
static enum AVPixelFormat getSupportedPixelFormat(AVCodecContext *s, const enum AVPixelFormat *pix_fmts);

class VideoPlayer {
public:
    VideoPlayer();
    ~VideoPlayer();

public:
    void initialize();
    bool open(std::string& fileName, bool useH264 = false);
    AVCodecContext* openVideoStream();
    AVCodecContext* openAudioStream();
    void update();

private:
    bool m_hasEglCreateImage = false;
    bool m_flushing = false;
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