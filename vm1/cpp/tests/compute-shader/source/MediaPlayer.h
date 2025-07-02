#pragma once

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

class MediaPlayer {

public:
    MediaPlayer();
    virtual ~MediaPlayer();
    
public:
    virtual bool openFile(const std::string& fileName = std::string()) = 0;
    void play();
    void close();
    bool isPlaying() const { return m_isRunning; }
    virtual void update() = 0;
    GLuint texture();

protected:
    void createVertexBuffers();
    void initializeFramebufferAndTextures();
    virtual void loadShaders() = 0;
    virtual void startThread() = 0;
    virtual void run() = 0;
    virtual void customCleanup();
    virtual void reset();
    void pushFrame(VideoFrame& frame);
    bool popFrame(VideoFrame& frame);
    bool peekFrame(VideoFrame& frame);

private: 
    void cleanup();
    void clearFrames();

protected:
    std::atomic<bool> m_isRunning = false;
    int m_numberOfInputImages = 1;

    // Rendering
    GLuint m_vao; 
    GLuint m_vbo;

    // Threading & Synchronization
    std::thread m_decoderThread;
    std::mutex m_frameMutex;
    std::condition_variable m_frameCV;
    EGLSyncKHR m_fence = EGL_NO_SYNC;
    
    // Textures & Shader
    GLuint m_frameBuffer = 0;
    GLuint m_rgbTexture = 0;
    Shader m_shader;

    // Frames
    std::vector<GLuint> m_yuvTextures;
    std::vector<EGLImage> m_yuvImages;

    static constexpr size_t MAX_QUEUE_SIZE = 3;
    VideoFrame m_currentFrame;
    std::queue<VideoFrame> m_frameQueue;
    std::vector<VideoFrame> m_framesToDelete;
};