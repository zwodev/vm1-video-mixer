#include "MediaPlayer.h"

#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengles2.h>
#include <EGL/eglext.h>
#include <GLES3/gl31.h>

MediaPlayer::MediaPlayer()
{
}

MediaPlayer::~MediaPlayer()
{
    close();
}

void MediaPlayer::play()
{
    reset();
    m_isRunning = true;
    startThread();
    //m_decoderThread = std::thread(&MediaPlayer::run, this);
}

GLuint MediaPlayer::texture()
{
    return m_rgbTexture;
}

void MediaPlayer::close()
{
    m_isRunning = false;
    m_frameCV.notify_all();
    
    if (m_decoderThread.joinable()) {
        m_decoderThread.join();
    }
    
    cleanup();
}

void MediaPlayer::cleanup()
{
    EGLDisplay display = eglGetCurrentDisplay();
    
    // Cleanup sync fences
    // Wait for fence and delete it
    if (m_fence != EGL_NO_SYNC) {
        eglClientWaitSync(display, m_fence, EGL_SYNC_FLUSH_COMMANDS_BIT, EGL_FOREVER);
        eglDestroySync(display, m_fence);
        m_fence = EGL_NO_SYNC;
    }

    clearFrames();
    customCleanup();
}

void MediaPlayer::reset()
{
    //
}

void MediaPlayer::customCleanup()
{
    //
}

void MediaPlayer::createVertexBuffers()
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

void MediaPlayer::initializeFramebufferAndTextures()
{
    // Generate input buffer (SAND128 NV12 / YUV)
    for (int i = 0; i < m_numberOfInputImages; ++i) {
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

void MediaPlayer::clearFrames() {
    std::unique_lock<std::mutex> lock(m_frameMutex);
    while (!m_frameQueue.empty()) {
        m_frameQueue.pop();
    }
}

void MediaPlayer::pushFrame(VideoFrame& frame) {
    std::unique_lock<std::mutex> lock(m_frameMutex);
    m_frameCV.wait(lock, [this]() { 
        return m_frameQueue.size() < MAX_QUEUE_SIZE || !m_isRunning; 
    });
    
    if (m_isRunning) {
        m_frameQueue.push(frame);
        m_frameCV.notify_one();
    }
}

bool MediaPlayer::popFrame(VideoFrame& frame) {
    std::unique_lock<std::mutex> lock(m_frameMutex);
    if (m_frameQueue.empty()) {
        return false;
    }
    
    frame = m_frameQueue.front();
    m_frameQueue.pop();
    m_frameCV.notify_one();
    return true;
}

bool MediaPlayer::peekFrame(VideoFrame& frame) {
    std::unique_lock<std::mutex> lock(m_frameMutex);
    if (m_frameQueue.empty()) {
        return false;
    }
    
    frame = m_frameQueue.front();
    return true;
}