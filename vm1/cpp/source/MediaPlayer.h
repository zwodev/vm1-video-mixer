#pragma once

#include "Shader.h"
#include "ThreadableQueue.h"
#include "AudioDevice.h"

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

struct VideoFrame {
    EGLImage image = EGL_NO_IMAGE;

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

struct AudioFrame {
    bool isFirstFrame = false;
    double pts = 0.0;
    SDL_AudioSpec spec = {0};
    std::vector<Uint8> data;
};

class MediaPlayer {

public:
    MediaPlayer();
    virtual ~MediaPlayer();
    
public:
    virtual bool openFile(const std::string& fileName = std::string(), AudioStream* audioStream = nullptr) = 0;
    void play();
    void close();
    bool isPlaying() const { return m_isRunning; }
    virtual void update() = 0;
    bool isFrameReady();
    GLuint texture();

protected:
    void createVertexBuffers();
    void initializeFramebufferAndTextures();
    virtual void loadShaders() = 0;
    virtual void run() = 0;
    virtual void customCleanup();
    virtual void reset();

private: 
    void cleanup();
    void clearFrames();

protected:
    std::atomic<bool> m_isRunning = false;
    int m_numberOfInputImages = 1;

    GLuint m_vao; 
    GLuint m_vbo;

    AudioStream* m_audio = nullptr;

    std::thread m_decoderThread;
    ThreadableQueue<VideoFrame> m_videoQueue;
    ThreadableQueue<AudioFrame> m_audioQueue;
    EGLSyncKHR m_fence = EGL_NO_SYNC;
    
    GLuint m_frameBuffer = 0;
    GLuint m_rgbTexture = 0;
    Shader m_shader;

    std::vector<GLuint> m_yuvTextures;
    std::vector<EGLImage> m_yuvImages;
};