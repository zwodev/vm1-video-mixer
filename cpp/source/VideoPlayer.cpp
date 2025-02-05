/*
 * Copyright (c) 2023-2024 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 * 
 * Parts of this file have been taken from:
 * https://github.com/libsdl-org/SDL/blob/main/test/testffmpeg.c
 * 
 */


#include "VideoPlayer.h"


#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengles2.h>
#include <EGL/eglext.h>



#ifndef fourcc_code
#define fourcc_code(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
#endif
#ifndef DRM_FORMAT_R8
#define DRM_FORMAT_R8 fourcc_code('R', '8', ' ', ' ')
#endif
#ifndef DRM_FORMAT_GR88
#define DRM_FORMAT_GR88 fourcc_code('G', 'R', '8', '8')
#endif

VideoPlayer::VideoPlayer()
{
}

VideoPlayer::~VideoPlayer()
{
    close();
}

void VideoPlayer::close()
{
    m_shouldStop = true;
    m_frameCV.notify_all();
    
    if (m_decoderThread.joinable()) {
        m_decoderThread.join();
    }
    
    cleanupResources();
}

bool VideoPlayer::open(std::string fileName, bool useH264)
{
    close(); // Cleanup any existing resources
    
    /* Open the media file */
    int result = avformat_open_input(&m_formatContext, fileName.c_str(), NULL, NULL);
    if (result < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't open %s: %d", fileName.c_str(), result);
        return false;
    }

    m_videoStream = av_find_best_stream(m_formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &m_videoCodec, 0);
    if (m_videoStream >= 0) {
        if (useH264) {
            const char *videoCodecName = "h264_v4l2m2m";
            m_videoCodec = avcodec_find_decoder_by_name(videoCodecName);
            if (!m_videoCodec) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't find codec '%s'", videoCodecName);
                return false;
            }
        }
        m_videoContext = openVideoStream();
        if (!m_videoContext) {
            return false;
        }
    }
    
    m_audioStream = av_find_best_stream(m_formatContext, AVMEDIA_TYPE_AUDIO, -1, m_videoStream, &m_audioCodec, 0);

    if (m_audioStream >= 0) {
        m_audioContext = openAudioStream();
        if (!m_audioContext) {
            return false;
        }
    }

    m_packet = av_packet_alloc();
    if (!m_packet) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "av_packet_alloc failed");
        return false;
    }
    m_frame = av_frame_alloc();
    if (!m_frame) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "av_frame_alloc failed");
        return false;
    }

    if (/* successful open */) {
        m_shouldStop = false;
        m_isPlaying = true;
        m_decoderThread = std::thread(&VideoPlayer::decodingThread, this);
        return true;
    }
    return false;
}

AVCodecContext* VideoPlayer::openVideoStream()
{
    AVStream *st = m_formatContext->streams[m_videoStream];
    AVCodecParameters *codecpar = st->codecpar;
    AVCodecContext *context;
    const AVCodecHWConfig *config;
    enum AVHWDeviceType type;
    int i;
    int result;

    SDL_Log("Video stream: %s %dx%d\n", avcodec_get_name(m_videoCodec->id), codecpar->width, codecpar->height);

    context = avcodec_alloc_context3(NULL);
    if (!context) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "avcodec_alloc_context3 failed");
        return NULL;
    }

    result = avcodec_parameters_to_context(context, m_formatContext->streams[m_videoStream]->codecpar);
    if (result < 0) {
        //SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "avcodec_parameters_to_context failed: %s\n", av_err2str(result));
        avcodec_free_context(&context);
        return NULL;
    }
    context->pkt_timebase = m_formatContext->streams[m_videoStream]->time_base;

    /* Look for supported hardware accelerated configurations */
    i = 0;
    while (!context->hw_device_ctx &&
           (config = avcodec_get_hw_config(m_videoCodec, i++)) != NULL) {
//#if 0
        SDL_Log("Found %s hardware acceleration with pixel format %s\n", av_hwdevice_get_type_name(config->device_type), av_get_pix_fmt_name(config->pix_fmt));
//#endif

        if (!(config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) ||
            !isSupportedPixelFormat(config->pix_fmt)) {
            continue;
        }

        type = AV_HWDEVICE_TYPE_NONE;
        while (!context->hw_device_ctx &&
               (type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE) {
            if (type != config->device_type) {
                continue;
            }

            {
                result = av_hwdevice_ctx_create(&context->hw_device_ctx, type, NULL, NULL, 0);
                if (result < 0) {
                    //SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create hardware device context: %s", av_err2str(result));
                } else {
                    SDL_Log("Using %s hardware acceleration with pixel format %s\n", av_hwdevice_get_type_name(config->device_type), av_get_pix_fmt_name(config->pix_fmt));
                }
            }
        }
    }

    /* Allow supported hardware accelerated pixel formats */
    context->get_format = getSupportedPixelFormat;

    result = avcodec_open2(context, m_videoCodec, NULL);
    if (result < 0) {
        //SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't open codec %s: %s", avcodec_get_name(context->codec_id), av_err2str(result));
        avcodec_free_context(&context);
        return nullptr;
    }

    //FIXME_SDL_SetWindowSize(window, codecpar->width, codecpar->height);

    return context;
}

AVCodecContext* VideoPlayer::openAudioStream()
{
    AVStream *st = m_formatContext->streams[m_audioStream];
    AVCodecParameters *codecpar = st->codecpar;
    AVCodecContext *context;
    int result;

    SDL_Log("Audio stream: %s %d channels, %d Hz\n", avcodec_get_name(m_audioCodec->id), codecpar->ch_layout.nb_channels, codecpar->sample_rate);

    context = avcodec_alloc_context3(nullptr);
    if (!context) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "avcodec_alloc_context3 failed\n");
        return nullptr;
    }

    result = avcodec_parameters_to_context(context, m_formatContext->streams[m_audioStream]->codecpar);
    if (result < 0) {
       // SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "avcodec_parameters_to_context failed: %s\n", av_err2str(result));
        avcodec_free_context(&context);
        return nullptr;
    }
    context->pkt_timebase = m_formatContext->streams[m_audioStream]->time_base;

    result = avcodec_open2(context, m_audioCodec, NULL);
    if (result < 0) {
        //SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't open codec %s: %s", avcodec_get_name(context->codec_id), av_err2str(result));
        avcodec_free_context(&context);
        return nullptr;
    }

    SDL_AudioSpec spec = { SDL_AUDIO_F32, codecpar->ch_layout.nb_channels, codecpar->sample_rate };
    m_audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (m_audio) {
        SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(m_audio));
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't open audio: %s", SDL_GetError());
    }

    return context;
}

void VideoPlayer::decodingThread() {
    while (!m_shouldStop) {
        if (!m_flushing) {
            // Read and decode frames
            int result = av_read_frame(m_formatContext, m_packet);
            if (result < 0) {
                SDL_Log("End of stream, finishing decode\n");
                if (m_audioContext) {
                    avcodec_flush_buffers(m_audioContext);
                }
                if (m_videoContext) {
                    avcodec_flush_buffers(m_videoContext);
                }
                m_flushing = true;
            } else {
                if (m_packet->stream_index == m_audioStream) {
                    result = avcodec_send_packet(m_audioContext, m_packet);
                    if (result < 0) {
                        //SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "avcodec_send_packet(audio_context) failed: %s", av_err2str(result));
                    }
                } else if (m_packet->stream_index == m_videoStream) {
                    result = avcodec_send_packet(m_videoContext, m_packet);
                    if (result < 0) {
                        //SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "avcodec_send_packet(video_context) failed: %s", av_err2str(result));
                    }
                }
                av_packet_unref(m_packet);
            }
        }
        
        // Process decoded frames
        if (m_videoContext) {
            while (avcodec_receive_frame(m_videoContext, m_frame) >= 0) {
                double pts = ((double)m_frame->pts * m_videoContext->pkt_timebase.num) / m_videoContext->pkt_timebase.den;
                if (m_firstPts < 0.0) {
                    m_firstPts = pts;
                }
                pts -= m_firstPts;

                VideoFrame frame;
                if (getTextureForDRMFrame(m_frame)) {
                    frame.images = std::move(m_images);
                    frame.pts = pts;
                    pushFrame(std::move(frame));
                }
            }
        }
    }
}

void VideoPlayer::update() {
    VideoFrame frame;
    if (popFrame(frame)) {
        // Create sync fence for previous frame if exists
        if (!m_fences.empty()) {
            EGLDisplay display = eglGetCurrentDisplay();
            eglClientWaitSync(display, m_fences.back(), EGL_SYNC_FLUSH_COMMANDS_BIT, EGL_FOREVER);
            eglDestroySync(display, m_fences.back());
            m_fences.pop_back();
        }
        
        // Render new frame
        m_planeRenderer.update(frame.images[0]);
        
        // Create new sync fence
        EGLDisplay display = eglGetCurrentDisplay();
        EGLSyncKHR fence = eglCreateSync(display, EGL_SYNC_FENCE, NULL);
        if (fence != EGL_NO_SYNC) {
            m_fences.push_back(fence);
        }
    }
}

void VideoPlayer::pushFrame(VideoFrame&& frame) {
    std::unique_lock<std::mutex> lock(m_frameMutex);
    m_frameCV.wait(lock, [this]() { 
        return m_frameQueue.size() < MAX_QUEUE_SIZE || m_shouldStop; 
    });
    
    if (!m_shouldStop) {
        m_frameQueue.push(std::move(frame));
        m_frameCV.notify_one();
    }
}

bool VideoPlayer::popFrame(VideoFrame& frame) {
    std::unique_lock<std::mutex> lock(m_frameMutex);
    if (m_frameQueue.empty()) {
        return false;
    }
    
    frame = std::move(m_frameQueue.front());
    m_frameQueue.pop();
    m_frameCV.notify_one();
    return true;
}

void VideoPlayer::cleanupResources() {
    EGLDisplay display = eglGetCurrentDisplay();
    
    // Cleanup sync fences
    for (auto fence : m_fences) {
        if (fence != EGL_NO_SYNC) {
            eglDestroySync(display, fence);
        }
    }
    m_fences.clear();
    
    // Cleanup images
    for (auto image : m_images) {
        eglDestroyImage(display, image);
    }
    m_images.clear();
    
    // ... cleanup other resources ...
}

void VideoPlayer::handleVideoFrame(AVFrame *frame, double pts)
{
    /* Quick and dirty PTS handling */
    if (!m_videoStart) {
        m_videoStart = SDL_GetTicks();
    }
    double now = (double)(SDL_GetTicks() - m_videoStart) / 1000.0;
    while (now < pts - 0.001) {
        SDL_Delay(1);
        now = (double)(SDL_GetTicks() - m_videoStart) / 1000.0;
    }

    displayVideoTexture(frame);
}

void VideoPlayer::displayVideoTexture(AVFrame *frame)
{
    /* Update the video texture */
    if (!getTextureForFrame(frame)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't get texture for frame: %s\n", SDL_GetError());
        return;
    }

    if (frame->linesize[0] < 0) {
        //SDL_RenderTextureRotated(m_renderer, m_videoTexture, NULL, NULL, 0.0, NULL, SDL_FLIP_VERTICAL);
    } else {

        //m_planeRenderer.update(m_videoTexture);

        //SDL_RenderTexture(m_renderer, m_videoTexture, NULL, NULL); 

        // int width = frame->width;
        // int height = frame->height;
        // SDL_Texture* oldRenderTarget = SDL_GetRenderTarget(m_renderer);

        // if (!m_renderTexture) {
        //     m_renderTexture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, width, height);
        // }
        // else {
        //     int oldWidth, oldHeight;
        //     SDL_QueryTexture(m_renderTexture, NULL, NULL, &oldWidth, &oldHeight);
        //     if (oldWidth != width || oldHeight != height) {
        //         m_renderTexture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, width, height);
        //     }
        // }
        //SDL_SetRenderTarget(m_renderer, m_renderTexture);
        //SDL_RenderTexture(m_renderer, m_videoTexture, NULL, NULL);   
        //SDL_SetRenderTarget(m_renderer, oldRenderTarget); 
    }
}

bool VideoPlayer::getTextureForFrame(AVFrame *frame)
{
    // switch (frame->format) {
    // case AV_PIX_FMT_VAAPI:
    //     return getTextureForVAAPIFrame(frame, texture);
    // case AV_PIX_FMT_DRM_PRIME:
        return getTextureForDRMFrame(frame);
    // default:
    //     return getTextureForMemoryFrame(frame, texture);
    // }
}

bool VideoPlayer::getTextureForMemoryFrame(AVFrame *frame)
{
    SDL_SetError("TextureForMemoryFrame is not supported!");
    return false;
}

bool VideoPlayer::getTextureForVAAPIFrame(AVFrame *frame)
{
    AVFrame *drm_frame;
    bool result = false;

    drm_frame = av_frame_alloc();
    if (drm_frame) {
        drm_frame->format = AV_PIX_FMT_DRM_PRIME;
        if (av_hwframe_map(drm_frame, frame, 0) == 0) {
            result = getTextureForDRMFrame(drm_frame);
        } else {
            SDL_SetError("Couldn't map hardware frame");
        }
        av_frame_free(&drm_frame);
    } else {
        SDL_OutOfMemory();
    }
    return result;
}

bool VideoPlayer::getTextureForDRMFrame(AVFrame *frame)
{
    const AVDRMFrameDescriptor *desc = (const AVDRMFrameDescriptor *)frame->data[0];
    EGLDisplay display = eglGetCurrentDisplay();

    /* FIXME: Assuming NV12 data format */
    int numPlanes = 0;
    for (int i = 0; i < desc->nb_layers; ++i) {
        numPlanes += desc->layers[i].nb_planes;
    }

    int newWidth = 2048;
    int newHeight = 2048;

    static const EGLint dma_fd[3] = {
		EGL_DMA_BUF_PLANE0_FD_EXT,
		EGL_DMA_BUF_PLANE1_FD_EXT,
		EGL_DMA_BUF_PLANE2_FD_EXT,
	};
	static const EGLint dma_offset[3] = {
		EGL_DMA_BUF_PLANE0_OFFSET_EXT,
		EGL_DMA_BUF_PLANE1_OFFSET_EXT,
		EGL_DMA_BUF_PLANE2_OFFSET_EXT,
	};
	static const EGLint dma_pitch[3] = {
		EGL_DMA_BUF_PLANE0_PITCH_EXT,
		EGL_DMA_BUF_PLANE1_PITCH_EXT,
		EGL_DMA_BUF_PLANE2_PITCH_EXT,
	};

    for (int i = 0; i < m_images.size(); ++i) {
        eglDestroyImage(display, m_images[i]);
    }

    m_images.clear();

    int imageIndex = 0;
    /* import the frame into OpenGL */
    for (int i = 0; i < desc->nb_layers; ++i) {
        const AVDRMLayerDescriptor *layer = &desc->layers[i];
        for (int j = 0; j < 1; ++j) {
            static const uint32_t formats[ 2 ] = { DRM_FORMAT_R8, DRM_FORMAT_GR88 };
            const AVDRMPlaneDescriptor *plane = &layer->planes[j];
            const AVDRMObjectDescriptor *object = &desc->objects[plane->object_index];
            
            EGLAttrib img_attr[] = {
                EGL_LINUX_DRM_FOURCC_EXT,      formats[j],
                EGL_WIDTH,                     newWidth  / ( imageIndex + 1 ),  /* half size for chroma */
                EGL_HEIGHT,                    newHeight / ( imageIndex + 1 ),
                dma_fd[j],                     object->fd,
                dma_offset[j],                 plane->offset,
                dma_pitch[j],                  newWidth,
                EGL_NONE
            };

            
            EGLImage pImage = eglCreateImage(display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, img_attr);
            m_images.push_back(pImage);
            imageIndex++;
        }
    }

    return true;
}

// static void setYUVConversionMode(AVFrame *frame)
// {
//     SDL_YUV_CONVERSION_MODE mode = SDL_YUV_CONVERSION_AUTOMATIC;
//     if (frame && (frame->format == AV_PIX_FMT_YUV420P || frame->format == AV_PIX_FMT_YUYV422 || frame->format == AV_PIX_FMT_UYVY422)) {
//         if (frame->color_range == AVCOL_RANGE_JPEG)
//             mode = SDL_YUV_CONVERSION_JPEG;
//         else if (frame->colorspace == AVCOL_SPC_BT709)
//             mode = SDL_YUV_CONVERSION_BT709;
//         else if (frame->colorspace == AVCOL_SPC_BT470BG || frame->colorspace == AVCOL_SPC_SMPTE170M)
//             mode = SDL_YUV_CONVERSION_BT601;
//     }
//     SDL_SetYUVConversionMode(mode); /* FIXME: no support for linear transfer */
// }

static SDL_PixelFormat getTextureFormat(enum AVPixelFormat format)
{
    switch (format) {
    case AV_PIX_FMT_RGB8:
        return SDL_PIXELFORMAT_RGB332;
    case AV_PIX_FMT_RGB444:
        return SDL_PIXELFORMAT_XRGB4444;
    case AV_PIX_FMT_RGB555:
        return SDL_PIXELFORMAT_XRGB1555;
    case AV_PIX_FMT_BGR555:
        return SDL_PIXELFORMAT_XBGR1555;
    case AV_PIX_FMT_RGB565:
        return SDL_PIXELFORMAT_RGB565;
    case AV_PIX_FMT_BGR565:
        return SDL_PIXELFORMAT_BGR565;
    case AV_PIX_FMT_RGB24:
        return SDL_PIXELFORMAT_RGB24;
    case AV_PIX_FMT_BGR24:
        return SDL_PIXELFORMAT_BGR24;
    case AV_PIX_FMT_0RGB32:
        return SDL_PIXELFORMAT_XRGB8888;
    case AV_PIX_FMT_0BGR32:
        return SDL_PIXELFORMAT_XBGR8888;
    case AV_PIX_FMT_NE(RGB0, 0BGR):
        return SDL_PIXELFORMAT_RGBX8888;
    case AV_PIX_FMT_NE(BGR0, 0RGB):
        return SDL_PIXELFORMAT_BGRX8888;
    case AV_PIX_FMT_RGB32:
        return SDL_PIXELFORMAT_ARGB8888;
    case AV_PIX_FMT_RGB32_1:
        return SDL_PIXELFORMAT_RGBA8888;
    case AV_PIX_FMT_BGR32:
        return SDL_PIXELFORMAT_ABGR8888;
    case AV_PIX_FMT_BGR32_1:
        return SDL_PIXELFORMAT_BGRA8888;
    case AV_PIX_FMT_YUV420P:
        return SDL_PIXELFORMAT_IYUV;
    case AV_PIX_FMT_YUYV422:
        return SDL_PIXELFORMAT_YUY2;
    case AV_PIX_FMT_UYVY422:
        return SDL_PIXELFORMAT_UYVY;
    case AV_PIX_FMT_NV12:
        return SDL_PIXELFORMAT_NV12;
    case AV_PIX_FMT_NV21:
        return SDL_PIXELFORMAT_NV21;
    case AV_PIX_FMT_P010:
        return SDL_PIXELFORMAT_P010;
    default:
        return SDL_PIXELFORMAT_UNKNOWN;
    }
}

static bool isSupportedPixelFormat(enum AVPixelFormat format)
{
    if (/* m_hasEglCreateImage && */
        (format == AV_PIX_FMT_VAAPI || format == AV_PIX_FMT_DRM_PRIME)) {
        return true;
    }

    if (getTextureFormat(format) != SDL_PIXELFORMAT_UNKNOWN) {
        return true;
    }
    return false;
}

static enum AVPixelFormat getSupportedPixelFormat(AVCodecContext *s, const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
        const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(*p);

        if (!(desc->flags & AV_PIX_FMT_FLAG_HWACCEL)) {
            /* We support all memory formats using swscale */
            break;
        }

        if (isSupportedPixelFormat(*p)) {
            /* We support this format */
            break;
        }
    }

    if (*p == AV_PIX_FMT_NONE) {
        SDL_Log("Couldn't find a supported pixel format:\n");
        for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
            SDL_Log("    %s\n", av_get_pix_fmt_name(*p));
        }
    }

    return *p;
}