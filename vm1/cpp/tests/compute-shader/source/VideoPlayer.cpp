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


#include "VideoPlayer.h"
#include "GLHelper.h"

#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengles2.h>
#include <EGL/eglext.h>
#include <GLES3/gl31.h>

#ifndef fourcc_code
#define fourcc_code(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
#endif
#ifndef DRM_FORMAT_R8
#define DRM_FORMAT_R8 fourcc_code('R', '8', ' ', ' ')
#endif
#ifndef DRM_FORMAT_GR88
#define DRM_FORMAT_GR88 fourcc_code('G', 'R', '8', '8')
#endif
#ifndef DRM_FORMAT_RGBA8888
#define DRM_FORMAT_RGBA8888 fourcc_code('R', 'A', '2', '4')
#endif

const int YUV_IMAGE_WIDTH = 2048;
const int YUV_IMAGE_HEIGHT = 1530;
 
VideoPlayer::VideoPlayer()
{
    loadShaders();
    createVertexBuffers();
    initializeFramebufferAndTextures();
}

VideoPlayer::~VideoPlayer()
{
    close();
}

void VideoPlayer::loadShaders()
{
    m_computeShader.load("shaders/pass.comp");
    m_shader.load("shaders/video.vert", "shaders/video.frag");
}

void VideoPlayer::createVertexBuffers()
{   
    float quadVertices[] = {
        //  x,    y,    u,   v
        -1.0f, -1.0f,  0.0f, 0.0f, // bottom left
        1.0f, -1.0f,  1.0f, 0.0f, // bottom right
        1.0f,  1.0f,  1.0f, 1.0f, // top right

        -1.0f, -1.0f,  0.0f, 0.0f, // bottom left
        1.0f,  1.0f,  1.0f, 1.0f, // top right
        -1.0f,  1.0f,  0.0f, 1.0f  // top left
    };

    // Setup VAO, VBO, and attribute pointers for aPos (vec2) and aTexCoord (vec2)
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
   
    // Texture coordinates
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void VideoPlayer::initializeFramebufferAndTextures()
{
    // Generate input buffer (SAND128 NV12 / YUV)
    for (int i = 0; i < 15; ++i) {
        GLuint texId;
        glGenTextures(1, &texId);
        m_yuvTextures.push_back(texId);
        m_yuvImages.push_back(nullptr);
    }
    
    // Generate and bind the output framebuffer (RGB)
    glGenFramebuffers(1, &m_frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

    // Create output texture (RGB)
    glGenTextures(1, &m_rgbTexture);
    glBindTexture(GL_TEXTURE_2D, m_rgbTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 1920, 1080);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_rgbTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void VideoPlayer::close()
{
    m_isRunning = false;
    m_frameCV.notify_all();
    
    if (m_decoderThread.joinable()) {
        m_decoderThread.join();
    }
    
    cleanupResources();
}

bool VideoPlayer::open(std::string fileName, bool useH264)
{
    // Cleanup any existing resources
    close(); 
    
    // This is just for rtsp steams. Necessary?
    AVDictionary* opts = NULL;
    av_dict_set(&opts, "rtsp_transport", "tcp", 0);

    // Open the video file
    int result = avformat_open_input(&m_formatContext, fileName.c_str(), NULL, &opts);
    if (result < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't open %s: %d", fileName.c_str(), result);
        return false;
    }

    if (avformat_find_stream_info(m_formatContext, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't find stream info in file %s: %d", fileName.c_str(), result);
        return false; 
    }

    bool foundStream = false;
    for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
        AVStream *stream = m_formatContext->streams[i];
        AVCodecParameters *codecpar = stream->codecpar;
        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
            codecpar->width == 1920 &&
            codecpar->height == 1080 &&
            codecpar->codec_id == AV_CODEC_ID_HEVC) {
            foundStream = true;
            break;
        }
    }

    if (!foundStream) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't find a valid HEVC/1080p video stream in file %s: %d", fileName.c_str(), result);
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
    
    // This is just for rtsp steams. Necessary?
    m_formatContext->max_analyze_duration = 5 * AV_TIME_BASE;
    m_formatContext->probesize = 5 * 1024 * 1024;
    //av_dump_format(m_formatContext, 0, "format_dump.txt", 0);


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

    return true;
}

void VideoPlayer::play()
{
    m_firstPts = -1.0;
    m_isFlushing = false;
    m_isRunning = true;
    m_decoderThread = std::thread(&VideoPlayer::decodingThread, this);
}

void VideoPlayer::setLooping(bool looping)
{
    m_isLooping = looping;
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
        
        SDL_Log("Found %s hardware acceleration with pixel format %s\n", av_hwdevice_get_type_name(config->device_type), av_get_pix_fmt_name(config->pix_fmt));

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

    // SDL_AudioSpec spec = { SDL_AUDIO_F32, codecpar->ch_layout.nb_channels, codecpar->sample_rate };
    // m_audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    // if (m_audio) {
    //     SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(m_audio));
    // } else {
    //     SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't open audio: %s", SDL_GetError());
    // }

    return context;
}

void VideoPlayer::render()
{
    glViewport(0, 0, 1920, 1080);
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_shader.activate();
    m_shader.setValue("stripWidthNDC", 2.0f/15.0f);

    glBindVertexArray(m_vao);
    for (int i = 0; i < m_yuvTextures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_yuvTextures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Bind the texture to unit
        glEGLImageTargetTexture2DOESFunc(GL_TEXTURE_2D, m_yuvImages[i]);
        m_shader.bindUniformLocation("inputTexture", 0);

        m_shader.setValue("stripId", i);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);
    m_shader.deactivate();

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void VideoPlayer::update()
{
    EGLDisplay display = eglGetCurrentDisplay();

    // Wait for fence and delete it
    eglClientWaitSync(display, m_fence, EGL_SYNC_FLUSH_COMMANDS_BIT, EGL_FOREVER);
    eglDestroySync(display, m_fence);
    m_fence = EGL_NO_SYNC;

    // TODO: Can EGLImages be reused? It seems like the DRM-Buf FDs change very often.
    for (auto& yuvImage : m_yuvImages ) {
        if (yuvImage != nullptr) {
            eglDestroyImage(display, yuvImage);
            yuvImage = nullptr;
        }
    }

    VideoFrame frame;
    
    // Check PTS
    if (peekFrame(frame)) {
        if (frame.isFirstFrame) {
            m_startTime  = SDL_GetTicks();
        }

        double pts = frame.pts;
        double now = (double)(SDL_GetTicks() - m_startTime) / 1000.0;
        
        // Do not pop and display frame when PTS is ahead 
        if (now < (pts - 0.001)) {
            //printf("Skipping frame because of PTS.\n");
            return;
        } 
    }
    
    // if (popFrame(frame)) {
    //     // Create EGL images here in the main thread
    //     // TODO: Support for multiple planes and images (see older version)
    //     if (frame.formats.size() > 0) {
    //         int j = 0;
    //         EGLAttrib img_attr[] = {
    //             EGL_LINUX_DRM_FOURCC_EXT,      frame.formats[j],
    //             EGL_WIDTH,                     frame.widths[j],
    //             EGL_HEIGHT,                    frame.heights[j],
    //             EGL_DMA_BUF_PLANE0_FD_EXT,     frame.fds[j],
    //             EGL_DMA_BUF_PLANE0_OFFSET_EXT, frame.offsets[j],
    //             EGL_DMA_BUF_PLANE0_PITCH_EXT,  frame.pitches[j],
    //             EGL_NONE
    //         };
            
    //         EGLImage image = eglCreateImage(display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, img_attr);
    //         if (image != EGL_NO_IMAGE) {
    //             m_yuvImage = image;
    //         }
    //     }
    // }

    if (popFrame(frame)) {
        // Create EGL images here in the main thread
        // TODO: Support for multiple planes and images (see older version)
        for (int i = 0; i < m_yuvImages.size(); ++i) {
            if (frame.formats.size() > 0) {
                int j = 0;
                EGLAttrib img_attr[] = {
                    EGL_LINUX_DRM_FOURCC_EXT,      frame.formats[j],
                    EGL_WIDTH,                     128,
                    EGL_HEIGHT,                    1632,
                    EGL_DMA_BUF_PLANE0_FD_EXT,     frame.fds[j],
                    EGL_DMA_BUF_PLANE0_OFFSET_EXT, i * 128 * 1632,
                    EGL_DMA_BUF_PLANE0_PITCH_EXT,  128,
                    EGL_NONE
                };
                
                EGLImage image = eglCreateImage(display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, img_attr);
                if (image != EGL_NO_IMAGE) {
                    m_yuvImages[i] = image;
                }
            }
        }
    }

    // TODO: Convert frame's texture using compute shader
    //runComputeShader();
    render();

    // Create fence for current frame
    m_fence = eglCreateSync(display, EGL_SYNC_FENCE, NULL);
}

void VideoPlayer::decodingThread() {
    while (m_isRunning) {
        if (!m_isFlushing) {
            // Read and decode frames
            int result = av_read_frame(m_formatContext, m_packet);
            if (result < 0) {
                if (m_isLooping) {
                    m_firstPts = -1.0;
                    av_seek_frame(m_formatContext, -1, 0, AVSEEK_FLAG_BACKWARD);
                    SDL_Log("End of stream, restart (looping)\n");
                }
                else {
                    SDL_Log("End of stream, finishing decode\n");     
                    m_isFlushing = true;
                }
                if (m_audioContext) {
                    avcodec_flush_buffers(m_audioContext);
                }
                if (m_videoContext) {
                    avcodec_flush_buffers(m_videoContext);
                }
            } else {
                if (m_packet->stream_index == m_audioStream) {
                    result = avcodec_send_packet(m_audioContext, m_packet);
                    if (result < 0) {
                        //return;
                    }
                } else if (m_packet->stream_index == m_videoStream) {
                    result = avcodec_send_packet(m_videoContext, m_packet);
                    if (result < 0) {
                        //return;
                    }
                }
                av_packet_unref(m_packet);
            }
        }
        
        // Process decoded frames
        if (m_videoContext) { 
            while (avcodec_receive_frame(m_videoContext, m_frame) >= 0) {
                double pts = ((double)m_frame->pts * m_videoContext->pkt_timebase.num) / m_videoContext->pkt_timebase.den;
                bool firstFrame = false;
                if (m_firstPts < 0.0) {
                    m_firstPts = pts;
                    firstFrame = true;
                }
                pts -= m_firstPts;

                VideoFrame frame;
                if (getTextureForDRMFrame(m_frame, frame)) {
                    frame.isFirstFrame = firstFrame;
                    frame.pts = pts;
                    pushFrame(frame);
                }
            }      
        }

        if (m_isFlushing) m_isRunning = false;
    }
}

void VideoPlayer::clearFrames() {
    std::unique_lock<std::mutex> lock(m_frameMutex);
    while (!m_frameQueue.empty()) {
        m_frameQueue.pop();
    }
}

void VideoPlayer::pushFrame(VideoFrame& frame) {
    std::unique_lock<std::mutex> lock(m_frameMutex);
    m_frameCV.wait(lock, [this]() { 
        return m_frameQueue.size() < MAX_QUEUE_SIZE || !m_isRunning; 
    });
    
    if (m_isRunning) {
        m_frameQueue.push(frame);
        m_frameCV.notify_one();
    }
}

bool VideoPlayer::popFrame(VideoFrame& frame) {
    std::unique_lock<std::mutex> lock(m_frameMutex);
    if (m_frameQueue.empty()) {
        return false;
    }
    
    frame = m_frameQueue.front();
    m_frameQueue.pop();
    m_frameCV.notify_one();
    return true;
}

bool VideoPlayer::peekFrame(VideoFrame& frame) {
    std::unique_lock<std::mutex> lock(m_frameMutex);
    if (m_frameQueue.empty()) {
        return false;
    }
    
    frame = m_frameQueue.front();
    return true;
}

GLuint VideoPlayer::texture()
{
    return m_rgbTexture;
}

void VideoPlayer::cleanupResources() {
    clearFrames();

    if (m_audioContext) {
        avcodec_free_context(&m_audioContext);
        m_audioContext = nullptr;
    }
    if (m_videoContext) {
        avcodec_free_context(&m_videoContext);
        m_videoContext = nullptr;
    }
    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
        avformat_free_context(m_formatContext);
        m_formatContext = nullptr;
    }
    m_audioCodec = nullptr;
    m_videoCodec = nullptr;
}

bool VideoPlayer::getTextureForDRMFrame(AVFrame *frame, VideoFrame &dstFrame)
{
    int newWidth = 2048;
    int newHeight = 1530;
    
    const AVDRMFrameDescriptor *desc = (const AVDRMFrameDescriptor *)frame->data[0];

    int imageIndex = 0;
    for (int i = 0; i < desc->nb_layers; ++i) {
        const AVDRMLayerDescriptor *layer = &desc->layers[i];
        for (int j = 0; j < layer->nb_planes; ++j) {
            static const uint32_t formats[2] = { DRM_FORMAT_R8, DRM_FORMAT_GR88 };
            const AVDRMPlaneDescriptor *plane = &layer->planes[j];
            const AVDRMObjectDescriptor *object = &desc->objects[plane->object_index];
            
            // Store DRM frame info instead of creating EGL image
            dstFrame.formats.push_back(formats[j]);
            dstFrame.widths.push_back(newWidth / (imageIndex + 1));
            dstFrame.heights.push_back(newHeight / (imageIndex + 1));
            dstFrame.fds.push_back(object->fd);
            dstFrame.offsets.push_back(plane->offset);
            dstFrame.pitches.push_back(newWidth);
            imageIndex++;
        }
    }
    return true;
}

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