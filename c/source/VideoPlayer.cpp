#include "VideoPlayer.h"

#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengles2.h>
#include <SDL3/SDL_egl.h>

VideoPlayer::VideoPlayer()
{
    initialize();
}

VideoPlayer::~VideoPlayer()
{
    
}

static SDL_bool has_EGL_EXT_image_dma_buf_import;
static PFNGLACTIVETEXTUREARBPROC glActiveTextureARBFunc;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOESFunc;
void VideoPlayer::initialize()
{
    const char *extensions = eglQueryString(eglGetCurrentDisplay(), EGL_EXTENSIONS);
    if (SDL_strstr(extensions, "EGL_EXT_image_dma_buf_import") != NULL) {
        has_EGL_EXT_image_dma_buf_import = SDL_TRUE;
    }

    if (SDL_GL_ExtensionSupported("GL_OES_EGL_image")) {
        glEGLImageTargetTexture2DOESFunc = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
    }

    glActiveTextureARBFunc = (PFNGLACTIVETEXTUREARBPROC)SDL_GL_GetProcAddress("glActiveTextureARB");

    if (has_EGL_EXT_image_dma_buf_import &&
        glEGLImageTargetTexture2DOESFunc &&
        glActiveTextureARBFunc) {
        m_hasEglCreateImage = true;
    }
}

bool VideoPlayer::open(std::string& fileName, bool useH264)
{
    /* Open the media file */
    int returnCode = -1;
    int result = avformat_open_input(&m_formatContext, fileName.c_str(), NULL, NULL);
    if (result < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't open %s: %d", fileName.c_str(), result);
        returnCode = 4;
        return false;
    }

    m_videoStream = av_find_best_stream(m_formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &m_videoCodec, 0);
    if (m_videoStream >= 0) {
        if (useH264) {
            const char *videoCodecName = "h264_v4l2m2m";
            m_videoCodec = avcodec_find_decoder_by_name(videoCodecName);
            if (!m_videoCodec) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't find codec '%s'", videoCodecName);
                returnCode = 4;
                return false;
            }
        }
        m_videoContext = openVideoStream();
        if (!m_videoContext) {
            returnCode = 4;
            return false;
        }
    }
    
    m_audioStream = av_find_best_stream(m_formatContext, AVMEDIA_TYPE_AUDIO, -1, m_videoStream, &m_audioCodec, 0);

    if (m_audioStream >= 0) {
        // if (audio_codec_name) {
        //     audio_codec = avcodec_find_decoder_by_name(audio_codec_name);
        //     if (!audio_codec) {
        //         SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't find codec '%s'", audio_codec_name);
        //         return_code = 4;
        //         goto quit;
        //     }
        // }
        m_audioContext = openAudioStream();
        if (!m_audioContext) {
            returnCode = 4;
            return false;
        }
    }

    m_packet = av_packet_alloc();
    if (!m_packet) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "av_packet_alloc failed");
        returnCode = 4;
        return false;
    }
    m_frame = av_frame_alloc();
    if (!m_frame) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "av_frame_alloc failed");
        returnCode = 4;
        return false;
    }
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
    SDL_AudioStream* audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_OUTPUT, &spec, NULL, NULL);
    if (audio) {
        SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(audio));
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't open audio: %s", SDL_GetError());
    }

    return context;
}

void VideoPlayer::update()
{
    if (!m_flushing) {
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

    bool decoded = false;
    if (m_audioContext) {
        while (avcodec_receive_frame(m_audioContext, m_frame) >= 0) {
            HandleAudioFrame(m_frame);
            decoded = true;
        }
        if (m_audioStream) {
            /* Let SDL know we're done sending audio */
            SDL_FlushAudioStream(audio);
        }
    }
    if (m_videoContext) {
        while (avcodec_receive_frame(m_videoContext, m_frame) >= 0) {
            double pts = ((double)m_frame->pts * m_videoContext->pkt_timebase.num) / m_videoContext->pkt_timebase.den;
            if (first_pts < 0.0) {
                first_pts = pts;
            }
            pts -= first_pts;

            HandleVideoFrame(m_frame, pts);
            decoded = true;
        }
    } else {
        /* Update video rendering */
        SDL_SetRenderDrawColor(renderer, 0xA0, 0xA0, 0xA0, 0xFF);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    if (m_flushing && !decoded) {
        if (SDL_GetAudioStreamQueued(audio) > 0) {
            /* Wait a little bit for the audio to finish */
            SDL_Delay(10);
        } else {
            done = 1;
        }
    }
}

static Uint32 getTextureFormat(enum AVPixelFormat format)
{
    switch (format) {
    case AV_PIX_FMT_RGB8:
        return SDL_PIXELFORMAT_RGB332;
    case AV_PIX_FMT_RGB444:
        return SDL_PIXELFORMAT_RGB444;
    case AV_PIX_FMT_RGB555:
        return SDL_PIXELFORMAT_RGB555;
    case AV_PIX_FMT_BGR555:
        return SDL_PIXELFORMAT_BGR555;
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
    default:
        return SDL_PIXELFORMAT_UNKNOWN;
    }
}

static bool isSupportedPixelFormat(enum AVPixelFormat format)
{
    if (/* m_hasEglCreateImage && */
        (format == AV_PIX_FMT_VAAPI || format == AV_PIX_FMT_DRM_PRIME)) {
        return SDL_TRUE;
    }

    if (getTextureFormat(format) != SDL_PIXELFORMAT_UNKNOWN) {
        return SDL_TRUE;
    }
    return SDL_FALSE;
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