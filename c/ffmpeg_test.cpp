/*
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/
/* Simple program:  Display a video with a sprite bouncing around over it
 *
 * For a more complete video example, see ffplay.c in the ffmpeg sources.
 */

#include <stdlib.h>
#include <time.h>
#include <iostream>

extern "C"
{
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_test.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_surface.h>
#include <SDL3_image/SDL_image.h>


#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include <libavutil/hwcontext_drm.h>


#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengles2.h>
#include <SDL3/SDL_egl.h>
}


// #ifdef av_err2str
// #undef av_err2str
// #include <string>
// av_always_inline std::string av_err2string(int errnum) {
//     char str[AV_ERROR_MAX_STRING_SIZE];
//     return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
// }
// #define av_err2str(err) av_err2string(err).c_str()
// #endif  // av_err2str

#ifndef fourcc_code
#define fourcc_code(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
#endif
#ifndef DRM_FORMAT_R8
#define DRM_FORMAT_R8 fourcc_code('R', '8', ' ', ' ')
#endif
#ifndef DRM_FORMAT_GR88
#define DRM_FORMAT_GR88 fourcc_code('G', 'R', '8', '8')
#endif

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_AudioStream *audio;
static SDL_Texture *video_texture;
static Uint64 video_start;
static SDL_bool software_only;
static SDL_bool has_eglCreateImage;

static SDL_bool has_EGL_EXT_image_dma_buf_import;
static PFNGLACTIVETEXTUREARBPROC glActiveTextureARBFunc;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOESFunc;

struct SwsContextContainer
{
    struct SwsContext *context;
};
static const char *SWS_CONTEXT_CONTAINER_PROPERTY = "SWS_CONTEXT_CONTAINER";
static int done;

static SDL_bool CreateWindowAndRenderer(Uint32 window_flags, const char *driver)
{
    SDL_RendererInfo info;
    SDL_bool useEGL = (driver && SDL_strcmp(driver, "opengles2") == 0);

    SDL_SetHint(SDL_HINT_RENDER_DRIVER, driver);
    if (useEGL) {
        SDL_SetHint(SDL_HINT_VIDEO_FORCE_EGL, "1");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    } else {
        SDL_SetHint(SDL_HINT_VIDEO_FORCE_EGL, "0");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    }
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);

    /* The window will be resized to the video size when it's loaded, in OpenVideoStream() */
    if (SDL_CreateWindowAndRenderer(320, 200, window_flags, &window, &renderer) < 0) {
        return SDL_FALSE;
    }

    if (SDL_GetRendererInfo(renderer, &info) == 0) {
        SDL_Log("Created renderer %s\n", info.name);
    }

    if (useEGL) {
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
            has_eglCreateImage = SDL_TRUE;
        }
    }
    return SDL_TRUE;
}

static Uint32 GetTextureFormat(enum AVPixelFormat format)
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

static SDL_bool SupportedPixelFormat(enum AVPixelFormat format)
{
    if (!software_only) {
        if (has_eglCreateImage &&
            (format == AV_PIX_FMT_VAAPI || format == AV_PIX_FMT_DRM_PRIME)) {
            return SDL_TRUE;
        }
    }

    if (GetTextureFormat(format) != SDL_PIXELFORMAT_UNKNOWN) {
        return SDL_TRUE;
    }
    return SDL_FALSE;
}

static enum AVPixelFormat GetSupportedPixelFormat(AVCodecContext *s, const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
        const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(*p);

        if (!(desc->flags & AV_PIX_FMT_FLAG_HWACCEL)) {
            /* We support all memory formats using swscale */
            break;
        }

        if (SupportedPixelFormat(*p)) {
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

static AVCodecContext *OpenVideoStream(AVFormatContext *ic, int stream, const AVCodec *codec)
{
    AVStream *st = ic->streams[stream];
    AVCodecParameters *codecpar = st->codecpar;
    AVCodecContext *context;
    const AVCodecHWConfig *config;
    enum AVHWDeviceType type;
    int i;
    int result;

    SDL_Log("Video stream: %s %dx%d\n", avcodec_get_name(codec->id), codecpar->width, codecpar->height);

    context = avcodec_alloc_context3(NULL);
    if (!context) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "avcodec_alloc_context3 failed");
        return NULL;
    }

    result = avcodec_parameters_to_context(context, ic->streams[stream]->codecpar);
    if (result < 0) {
        //SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "avcodec_parameters_to_context failed: %s\n", av_err2str(result));
        avcodec_free_context(&context);
        return NULL;
    }
    context->pkt_timebase = ic->streams[stream]->time_base;

    /* Look for supported hardware accelerated configurations */
    i = 0;
    while (!context->hw_device_ctx &&
           (config = avcodec_get_hw_config(codec, i++)) != NULL) {
//#if 0
        SDL_Log("Found %s hardware acceleration with pixel format %s\n", av_hwdevice_get_type_name(config->device_type), av_get_pix_fmt_name(config->pix_fmt));
//#endif

        if (!(config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) ||
            !SupportedPixelFormat(config->pix_fmt)) {
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
    context->get_format = GetSupportedPixelFormat;

    result = avcodec_open2(context, codec, NULL);
    if (result < 0) {
        //SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't open codec %s: %s", avcodec_get_name(context->codec_id), av_err2str(result));
        avcodec_free_context(&context);
        return NULL;
    }

    SDL_SetWindowSize(window, codecpar->width, codecpar->height);

    return context;
}

static void SetYUVConversionMode(AVFrame *frame)
{
    SDL_YUV_CONVERSION_MODE mode = SDL_YUV_CONVERSION_AUTOMATIC;
    if (frame && (frame->format == AV_PIX_FMT_YUV420P || frame->format == AV_PIX_FMT_YUYV422 || frame->format == AV_PIX_FMT_UYVY422)) {
        if (frame->color_range == AVCOL_RANGE_JPEG)
            mode = SDL_YUV_CONVERSION_JPEG;
        else if (frame->colorspace == AVCOL_SPC_BT709)
            mode = SDL_YUV_CONVERSION_BT709;
        else if (frame->colorspace == AVCOL_SPC_BT470BG || frame->colorspace == AVCOL_SPC_SMPTE170M)
            mode = SDL_YUV_CONVERSION_BT601;
    }
    SDL_SetYUVConversionMode(mode); /* FIXME: no support for linear transfer */
}

static void SDLCALL FreeSwsContextContainer(void *userdata, void *value)
{
    struct SwsContextContainer *sws_container = (struct SwsContextContainer *)value;
    if (sws_container->context) {
        sws_freeContext(sws_container->context);
    }
    SDL_free(sws_container);
}

static SDL_bool GetTextureForMemoryFrame(AVFrame *frame, SDL_Texture **texture)
{
    int texture_width = 0, texture_height = 0;
    Uint32 texture_format = SDL_PIXELFORMAT_UNKNOWN;
    Uint32 frame_format = GetTextureFormat(frame->format);

    if (*texture) {
        SDL_QueryTexture(*texture, &texture_format, NULL, &texture_width, &texture_height);
    }
    if (!*texture || texture_width != frame->width || texture_height != frame->height ||
        (frame_format != SDL_PIXELFORMAT_UNKNOWN && texture_format != frame_format) ||
        (frame_format == SDL_PIXELFORMAT_UNKNOWN && texture_format != SDL_PIXELFORMAT_ARGB8888)) {
        if (*texture) {
            SDL_DestroyTexture(*texture);
        }

        if (frame_format == SDL_PIXELFORMAT_UNKNOWN) {
            *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, frame->width, frame->height);
        } else {
            *texture = SDL_CreateTexture(renderer, frame_format, SDL_TEXTUREACCESS_STREAMING, frame->width, frame->height);
        }
        if (!*texture) {
            return SDL_FALSE;
        }

        if (frame_format == SDL_PIXELFORMAT_UNKNOWN || SDL_ISPIXELFORMAT_ALPHA(frame_format)) {
            SDL_SetTextureBlendMode(*texture, SDL_BLENDMODE_BLEND);
        } else {
            SDL_SetTextureBlendMode(*texture, SDL_BLENDMODE_NONE);
        }
        SDL_SetTextureScaleMode(*texture, SDL_SCALEMODE_LINEAR);
    }

    switch (frame_format) {
    case SDL_PIXELFORMAT_UNKNOWN:
    {
        SDL_PropertiesID props = SDL_GetTextureProperties(*texture);
        struct SwsContextContainer *sws_container = (struct SwsContextContainer *)SDL_GetProperty(props, SWS_CONTEXT_CONTAINER_PROPERTY);
        if (!sws_container) {
            sws_container = (struct SwsContextContainer *)SDL_calloc(1, sizeof(*sws_container));
            if (!sws_container) {
                SDL_OutOfMemory();
                return SDL_FALSE;
            }
            SDL_SetPropertyWithCleanup(props, SWS_CONTEXT_CONTAINER_PROPERTY, sws_container, FreeSwsContextContainer, NULL);
        }
        sws_container->context = sws_getCachedContext(sws_container->context, frame->width, frame->height, frame->format, frame->width, frame->height, AV_PIX_FMT_BGRA, SWS_POINT, NULL, NULL, NULL);
        if (sws_container->context) {
            uint8_t *pixels[4];
            int pitch[4];
            if (SDL_LockTexture(*texture, NULL, (void **)&pixels[0], &pitch[0]) == 0) {
                sws_scale(sws_container->context, (const uint8_t * const *)frame->data, frame->linesize, 0, frame->height, pixels, pitch);
                SDL_UnlockTexture(*texture);
            }
        } else {
            SDL_SetError("Can't initialize the conversion context");
            return SDL_FALSE;
        }
        break;
    }
    case SDL_PIXELFORMAT_IYUV:
        if (frame->linesize[0] > 0 && frame->linesize[1] > 0 && frame->linesize[2] > 0) {
            SDL_UpdateYUVTexture(*texture, NULL, frame->data[0], frame->linesize[0],
                                                   frame->data[1], frame->linesize[1],
                                                   frame->data[2], frame->linesize[2]);
        } else if (frame->linesize[0] < 0 && frame->linesize[1] < 0 && frame->linesize[2] < 0) {
            SDL_UpdateYUVTexture(*texture, NULL, frame->data[0] + frame->linesize[0] * (frame->height                    - 1), -frame->linesize[0],
                                                   frame->data[1] + frame->linesize[1] * (AV_CEIL_RSHIFT(frame->height, 1) - 1), -frame->linesize[1],
                                                   frame->data[2] + frame->linesize[2] * (AV_CEIL_RSHIFT(frame->height, 1) - 1), -frame->linesize[2]);
        }
        SetYUVConversionMode(frame);
        break;
    default:
        if (frame->linesize[0] < 0) {
            SDL_UpdateTexture(*texture, NULL, frame->data[0] + frame->linesize[0] * (frame->height - 1), -frame->linesize[0]);
        } else {
            SDL_UpdateTexture(*texture, NULL, frame->data[0], frame->linesize[0]);
        }
        break;
    }
    return SDL_TRUE;
}

static SDL_bool GetTextureForDRMFrame(AVFrame *frame, SDL_Texture **texture)
{
    const AVDRMFrameDescriptor *desc = (const AVDRMFrameDescriptor *)frame->data[0];
    int i, j, image_index, num_planes;
    EGLDisplay display = eglGetCurrentDisplay();

    /* FIXME: Assuming NV12 data format */
    num_planes = 0;
    for (i = 0; i < desc->nb_layers; ++i) {
        num_planes += desc->layers[i].nb_planes;
    }
    // if (num_planes != 2) {
    //     SDL_SetError("Expected NV12 frames with 2 planes, instead got %d planes", num_planes);
    //     return SDL_FALSE;
    // }

    if (*texture) {
        /* Free the previous texture now that we're about to render a new one */
        SDL_DestroyTexture(*texture);
    } else {
        /* First time set up for NV12 textures */
        SDL_SetHint("SDL_RENDER_OPENGL_NV12_RG_SHADER", "1");

        SetYUVConversionMode(frame);
    }

    int newWidth = 2048;
    int newHeight = 2048;
    //int newHeight = 1024;
    //std::cout << "W: " << frame->width << std::endl; 
    //std::cout << "H: " << frame->height << std::endl; 
    //*texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_NV12, SDL_TEXTUREACCESS_STATIC, frame->width, frame->height);
    *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_NV12, SDL_TEXTUREACCESS_STATIC, newWidth, newHeight);
    if (!*texture) {
        return SDL_FALSE;
    }
    SDL_SetTextureBlendMode(*texture, SDL_BLENDMODE_NONE);
    SDL_SetTextureScaleMode(*texture, SDL_SCALEMODE_LINEAR);

    /* Bind the texture for importing */
    SDL_GL_BindTexture(*texture, NULL, NULL);

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

    /* import the frame into OpenGL */
    image_index = 0;
    for (i = 0; i < desc->nb_layers; ++i) {
        const AVDRMLayerDescriptor *layer = &desc->layers[i];
        for (j = 0; j < 1; ++j) {
            static const uint32_t formats[ 2 ] = { DRM_FORMAT_R8, DRM_FORMAT_GR88 };
            const AVDRMPlaneDescriptor *plane = &layer->planes[j];
            const AVDRMObjectDescriptor *object = &desc->objects[plane->object_index];

            std::cout << "Plane: " << j << std::endl;
            std::cout << "FD: " << object->fd << std::endl;
            std::cout << "Offset: " << plane->offset << std::endl;
            std::cout << "Pitch: " << plane->pitch << std::endl;
            
            EGLAttrib img_attr[] = {
                EGL_LINUX_DRM_FOURCC_EXT,      formats[j],
                EGL_WIDTH,                     newWidth  / ( image_index + 1 ),  /* half size for chroma */
                EGL_HEIGHT,                    newHeight / ( image_index + 1 ),
                dma_fd[j],                     object->fd,
                dma_offset[j],                 plane->offset,
                dma_pitch[j],                  newWidth,
                EGL_NONE
            };
            EGLImage pImage = eglCreateImage(display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, img_attr);

            glActiveTextureARBFunc(GL_TEXTURE0_ARB + image_index);
            glEGLImageTargetTexture2DOESFunc(GL_TEXTURE_2D, pImage);
            ++image_index;
        }
    }

    SDL_GL_UnbindTexture(*texture);

    return SDL_TRUE;
}

static SDL_bool GetTextureForVAAPIFrame(AVFrame *frame, SDL_Texture **texture)
{
    AVFrame *drm_frame;
    SDL_bool result = SDL_FALSE;

    drm_frame = av_frame_alloc();
    if (drm_frame) {
        drm_frame->format = AV_PIX_FMT_DRM_PRIME;
        if (av_hwframe_map(drm_frame, frame, 0) == 0) {
            result = GetTextureForDRMFrame(drm_frame, texture);
        } else {
            SDL_SetError("Couldn't map hardware frame");
        }
        av_frame_free(&drm_frame);
    } else {
        SDL_OutOfMemory();
    }
    return result;
}

static SDL_bool GetTextureForFrame(AVFrame *frame, SDL_Texture **texture)
{
    switch (frame->format) {
    case AV_PIX_FMT_VAAPI:
        return GetTextureForVAAPIFrame(frame, texture);
    case AV_PIX_FMT_DRM_PRIME:
        return GetTextureForDRMFrame(frame, texture);
    default:
        return GetTextureForMemoryFrame(frame, texture);
    }
}

static void DisplayVideoTexture(AVFrame *frame)
{
    /* Update the video texture */
    if (!GetTextureForFrame(frame, &video_texture)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't get texture for frame: %s\n", SDL_GetError());
        return;
    }

    if (frame->linesize[0] < 0) {
        SDL_RenderTextureRotated(renderer, video_texture, NULL, NULL, 0.0, NULL, SDL_FLIP_VERTICAL);
    } else {
        // int width, height;
        // SDL_Texture* target = SDL_GetRenderTarget(renderer);
        // SDL_QueryTexture(video_texture, NULL, NULL, &width, &height);
        // SDL_Texture* renTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, width, height);
        // SDL_SetRenderTarget(renderer, renTex);
        // SDL_Surface* surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32);
        
        SDL_RenderTexture(renderer, video_texture, NULL, NULL);
        
        // SDL_RenderReadPixels(renderer, NULL, surface->format->format, surface->pixels, surface->pitch);   
        // int st = IMG_SavePNG(surface, "test_xy.png");
        // if (st != 0) {
        //     SDL_Log("Failed saving image: %s\n", SDL_GetError());
        // }
        // SDL_SetRenderTarget(renderer, target);    
        // SDL_DestroySurface(surface);
    }
}

static void DisplayVideoFrame(AVFrame *frame)
{
    switch (frame->format) {
    case AV_PIX_FMT_VIDEOTOOLBOX:
        //DisplayVideoToolbox(frame);
        break;
    default:
        DisplayVideoTexture(frame);
        break;
    }
}

static void HandleVideoFrame(AVFrame *frame, double pts)
{
    /* Quick and dirty PTS handling */
    if (!video_start) {
        video_start = SDL_GetTicks();
    }
    double now = (double)(SDL_GetTicks() - video_start) / 1000.0;
    while (now < pts - 0.001) {
        SDL_Delay(1);
        now = (double)(SDL_GetTicks() - video_start) / 1000.0;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    DisplayVideoFrame(frame);

    SDL_RenderPresent(renderer);
}

static AVCodecContext *OpenAudioStream(AVFormatContext *ic, int stream, const AVCodec *codec)
{
    AVStream *st = ic->streams[stream];
    AVCodecParameters *codecpar = st->codecpar;
    AVCodecContext *context;
    int result;

    SDL_Log("Audio stream: %s %d channels, %d Hz\n", avcodec_get_name(codec->id), codecpar->ch_layout.nb_channels, codecpar->sample_rate);

    context = avcodec_alloc_context3(NULL);
    if (!context) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "avcodec_alloc_context3 failed\n");
        return NULL;
    }

    result = avcodec_parameters_to_context(context, ic->streams[stream]->codecpar);
    if (result < 0) {
       // SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "avcodec_parameters_to_context failed: %s\n", av_err2str(result));
        avcodec_free_context(&context);
        return NULL;
    }
    context->pkt_timebase = ic->streams[stream]->time_base;

    result = avcodec_open2(context, codec, NULL);
    if (result < 0) {
        //SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't open codec %s: %s", avcodec_get_name(context->codec_id), av_err2str(result));
        avcodec_free_context(&context);
        return NULL;
    }

    SDL_AudioSpec spec = { SDL_AUDIO_F32, codecpar->ch_layout.nb_channels, codecpar->sample_rate };
    audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_OUTPUT, &spec, NULL, NULL);
    if (audio) {
        SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(audio));
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't open audio: %s", SDL_GetError());
    }
    return context;
}

static SDL_AudioFormat GetAudioFormat(enum AVSampleFormat format)
{
    switch (format) {
    case AV_SAMPLE_FMT_U8:
    case AV_SAMPLE_FMT_U8P:
        return SDL_AUDIO_U8;
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_S16P:
        return SDL_AUDIO_S16;
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_S32P:
        return SDL_AUDIO_S32;
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_FLTP:
        return SDL_AUDIO_F32;
    default:
        /* Unsupported */
        return 0;
    }
}

static SDL_bool IsPlanarAudioFormat(enum AVSampleFormat format)
{
    switch (format) {
    case AV_SAMPLE_FMT_U8P:
    case AV_SAMPLE_FMT_S16P:
    case AV_SAMPLE_FMT_S32P:
    case AV_SAMPLE_FMT_FLTP:
    case AV_SAMPLE_FMT_DBLP:
    case AV_SAMPLE_FMT_S64P:
        return SDL_TRUE;
    default:
        return SDL_FALSE;
    }
}

static void InterleaveAudio(AVFrame *frame, const SDL_AudioSpec *spec)
{
    int c, n;
    int samplesize = SDL_AUDIO_BYTESIZE(spec->format);
    int framesize = SDL_AUDIO_FRAMESIZE(*spec);
    Uint8 *data = (Uint8 *)SDL_malloc(frame->nb_samples * framesize);
    if (!data) {
        return;
    }

    /* This could be optimized with SIMD and not allocating memory each time */
    for (c = 0; c < spec->channels; ++c) {
        const Uint8 *src = frame->data[c];
        Uint8 *dst = data + c * samplesize;
        for (n = frame->nb_samples; n--; ) {
            SDL_memcpy(dst, src, samplesize);
            src += samplesize;
            dst += framesize;
        }
    }
    SDL_PutAudioStreamData(audio, data, frame->nb_samples * framesize);
    SDL_free(data);
}

static void HandleAudioFrame(AVFrame *frame)
{
    if (audio) {
        SDL_AudioSpec spec = { GetAudioFormat(frame->format), frame->ch_layout.nb_channels, frame->sample_rate };
        SDL_SetAudioStreamFormat(audio, &spec, NULL);

        if (frame->ch_layout.nb_channels > 1 && IsPlanarAudioFormat(frame->format)) {
            InterleaveAudio(frame, &spec);
        } else {
            SDL_PutAudioStreamData(audio, frame->data[0], frame->nb_samples * SDL_AUDIO_FRAMESIZE(spec));
        }
    }
}

static void print_usage(SDLTest_CommonState *state, const char *argv0) {
    static const char *options[] = { "[--sprites N]", "[--audio-codec codec]", "[--video-codec codec]", "[--software]", "video_file", NULL };
    SDLTest_CommonLogUsage(state, argv0, options);
}

int main(int argc, char *argv[])
{
    const char *file = NULL;
    AVFormatContext *ic = NULL;
    int audio_stream = -1;
    int video_stream = -1;
    const char *audio_codec_name = NULL;
    const char *video_codec_name = NULL;
    const AVCodec *audio_codec = NULL;
    const AVCodec *video_codec = NULL;
    AVCodecContext *audio_context = NULL;
    AVCodecContext *video_context = NULL;
    AVPacket *pkt = NULL;
    AVFrame *frame = NULL;
    double first_pts = -1.0;
    int i;
    int result;
    int return_code = -1;
    Uint32 window_flags;
    SDL_bool flushing = SDL_FALSE;
    SDL_bool decoded = SDL_FALSE;
    SDLTest_CommonState *state;

    /* Initialize test framework */
    state = SDLTest_CommonCreateState(argv, 0);

    /* Enable standard application logging */
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);


    /* Parse commandline */
    for (i = 1; i < argc;) {
        int consumed;

        consumed = SDLTest_CommonArg(state, i);
        if (!consumed) {
            if (SDL_strcmp(argv[i], "--audio-codec") == 0 && argv[i+1]) {
                audio_codec_name = argv[i+1];
                consumed = 2;
            } else if (SDL_strcmp(argv[i], "--video-codec") == 0 && argv[i+1]) {
                video_codec_name = argv[i+1];
                consumed = 2;
            } else if (SDL_strcmp(argv[i], "--software") == 0) {
                software_only = SDL_TRUE;
                consumed = 1;
            } else if (!file) {
                /* We'll try to open this as a media file */
                file = argv[i];
                consumed = 1;
            }
        }
        if (consumed <= 0) {
            print_usage(state, argv[0]);
            return_code = 1;
            goto quit;
        }

        i += consumed;
    }

    if (!file) {
        print_usage(state, argv[0]);
        return_code = 1;
        goto quit;
    }

    if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0) {
        return_code = 2;
        goto quit;
    }

    window_flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    window_flags |= SDL_WINDOW_OPENGL;
    /* Try to create an EGL compatible window for DRM hardware frame support */
    if (!window) {
        CreateWindowAndRenderer(window_flags, "opengles2");
    }

    if (!window) {
        if (!CreateWindowAndRenderer(window_flags, NULL)) {
            return_code = 2;
            goto quit;
        }
    }

    if (SDL_SetWindowTitle(window, file) < 0) {
        SDL_Log("SDL_SetWindowTitle: %s", SDL_GetError());
    }

    /* Open the media file */
    result = avformat_open_input(&ic, file, NULL, NULL);
    if (result < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't open %s: %d", argv[1], result);
        return_code = 4;
        goto quit;
    }
    video_stream = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, &video_codec, 0);
    if (video_stream >= 0) {
        if (video_codec_name) {
            video_codec = avcodec_find_decoder_by_name(video_codec_name);
            if (!video_codec) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't find codec '%s'", video_codec_name);
                return_code = 4;
                goto quit;
            }
        }
        video_context = OpenVideoStream(ic, video_stream, video_codec);
        if (!video_context) {
            return_code = 4;
            goto quit;
        }
    }
    audio_stream = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, video_stream, &audio_codec, 0);
    std::cout << "Check1" << std::endl;
    if (audio_stream >= 0) {
        if (audio_codec_name) {
            audio_codec = avcodec_find_decoder_by_name(audio_codec_name);
            if (!audio_codec) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't find codec '%s'", audio_codec_name);
                return_code = 4;
                goto quit;
            }
        }
        audio_context = OpenAudioStream(ic, audio_stream, audio_codec);
        if (!audio_context) {
            return_code = 4;
            goto quit;
        }
    }
    std::cout << "Check2" << std::endl;
    pkt = av_packet_alloc();
    if (!pkt) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "av_packet_alloc failed");
        return_code = 4;
        goto quit;
    }
    frame = av_frame_alloc();
    if (!frame) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "av_frame_alloc failed");
        return_code = 4;
        goto quit;
    }

    // /* Position sprites and set their velocities */
    // SDL_Rect viewport;
    // SDL_GetRenderViewport(renderer, &viewport);
    // srand((unsigned int)time(NULL));
    // for (i = 0; i < num_sprites; ++i) {
    //     positions[i].x = (float)(rand() % (viewport.w - sprite_w));
    //     positions[i].y = (float)(rand() % (viewport.h - sprite_h));
    //     positions[i].w = (float)sprite_w;
    //     positions[i].h = (float)sprite_h;
    //     velocities[i].x = 0.0f;
    //     velocities[i].y = 0.0f;
    //     while (!velocities[i].x || !velocities[i].y) {
    //         velocities[i].x = (float)((rand() % (2 + 1)) - 1);
    //         velocities[i].y = (float)((rand() % (2 + 1)) - 1);
    //     }
    // }

    /* We're ready to go! */
    SDL_ShowWindow(window);

    std::cout << "Check3" << std::endl;

    /* Main render loop */
    done = 0;

    while (!done) {
        SDL_Event event;

        /* Check for events */
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_KEY_DOWN) {
                done = 1;
            }
        }

        if (!flushing) {
            result = av_read_frame(ic, pkt);
            if (result < 0) {
                SDL_Log("End of stream, finishing decode\n");
                if (audio_context) {
                    avcodec_flush_buffers(audio_context);
                }
                if (video_context) {
                    avcodec_flush_buffers(video_context);
                }
                flushing = SDL_TRUE;
            } else {
                if (pkt->stream_index == audio_stream) {
                    result = avcodec_send_packet(audio_context, pkt);
                    if (result < 0) {
                        //SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "avcodec_send_packet(audio_context) failed: %s", av_err2str(result));
                    }
                } else if (pkt->stream_index == video_stream) {
                    result = avcodec_send_packet(video_context, pkt);
                    if (result < 0) {
                        //SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "avcodec_send_packet(video_context) failed: %s", av_err2str(result));
                    }
                }
                av_packet_unref(pkt);
            }
        }

        decoded = SDL_FALSE;
        if (audio_context) {
            while (avcodec_receive_frame(audio_context, frame) >= 0) {
                HandleAudioFrame(frame);
                decoded = SDL_TRUE;
            }
            if (flushing) {
                /* Let SDL know we're done sending audio */
                SDL_FlushAudioStream(audio);
            }
        }
        if (video_context) {
            while (avcodec_receive_frame(video_context, frame) >= 0) {
                double pts = ((double)frame->pts * video_context->pkt_timebase.num) / video_context->pkt_timebase.den;
                if (first_pts < 0.0) {
                    first_pts = pts;
                }
                pts -= first_pts;

                HandleVideoFrame(frame, pts);
                decoded = SDL_TRUE;
            }
        } else {
            /* Update video rendering */
            SDL_SetRenderDrawColor(renderer, 0xA0, 0xA0, 0xA0, 0xFF);
            SDL_RenderClear(renderer);
            SDL_RenderPresent(renderer);
        }

        if (flushing && !decoded) {
            if (SDL_GetAudioStreamQueued(audio) > 0) {
                /* Wait a little bit for the audio to finish */
                SDL_Delay(10);
            } else {
                done = 1;
            }
        }
    }
    return_code = 0;
quit:
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&audio_context);
    avcodec_free_context(&video_context);
    avformat_close_input(&ic);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    SDLTest_CommonDestroyState(state);
    return return_code;
}