/*
 * Copyright (c) 2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

// The Video Capture software stack explained
// https://xilinx.github.io/vck190-base-trd/2021.2/html/arch/arch-sw.html
// 
// How to get UYVY working
// https://forums.raspberrypi.com/viewtopic.php?t=359412
//
// Media Controller configuration script 
// https://github.com/FearL0rd/RPi5_hdmi_in_card/tree/main

#include "CameraPlayer.h"
#include "GLHelper.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <drm/drm_fourcc.h>

#include <iostream>
#include <math.h>

#include <SDL3/SDL.h>
#include <GLES3/gl31.h>

using namespace std;

const int IMAGE_WIDTH = 1920;
const int IMAGE_HEIGHT = 1080;


CameraPlayer::CameraPlayer()
{
    loadShaders();
    createVertexBuffers();
    initializeFramebufferAndTextures();
}

CameraPlayer::~CameraPlayer()
{
    close();  
}

bool CameraPlayer::openFile(const std::string& fileName, AudioStream* audioStream)
{

}

void CameraPlayer::close()
{
    finalize();
    MediaPlayer::close();
}

void CameraPlayer::finalize()
{
    if (m_fd < 0) return;

    // 1. Stop streaming if started
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_STREAMOFF, &type) == -1) {
        printf("Error stopping stream.\n");
        // Not returning false here, try to cleanup anyway
    }

    // 2. Release each exported buffer (close fd)
    for (auto& buffer : m_buffers) {
        dequeueBuffer(m_fd);
    }
    for (auto& buffer : m_buffers) {
        if (buffer.fd >= 0) {
            ::close(buffer.fd);
        }
    }
    m_buffers.clear();

    // 3. Release buffer allocation in driver
    struct v4l2_requestbuffers req = {0};
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    req.count = 0; // Tell kernel to release buffers
    if (ioctl(m_fd, VIDIOC_REQBUFS, &req) == -1) {
        printf("Error releasing buffers.\n");
        // Not returning false here, try to cleanup device anyway
    }

    // 4. Close the device
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

void CameraPlayer::lockBuffer()
{
    if (!m_isRunning) return;    
    m_bufferIndex = dequeueBuffer(m_fd);
}

Buffer* CameraPlayer::getBuffer()
{
    Buffer* buffer = nullptr;
    if (!m_isRunning) return buffer;    

    if (m_bufferIndex >= 0) {
        buffer = &(m_buffers[m_bufferIndex]);
    }

    return buffer;
}
    
void CameraPlayer::unlockBuffer()
{
    if (!m_isRunning) return;

    if (m_bufferIndex >= 0) {
        //printf("Enqueue! \n");
        queueBuffer(m_fd, m_bufferIndex);
        m_bufferIndex = -1;
    }
}

bool CameraPlayer::setFormat(int fd)
{
    struct v4l2_format fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = IMAGE_WIDTH;
	fmt.fmt.pix.height      = IMAGE_HEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
	fmt.fmt.pix.field       = V4L2_FIELD_NONE;
	
    // Try setting this format
	ioctl(fd, VIDIOC_S_FMT, &fmt);

	// Check what was actually set
	if(fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_UYVY)
	{
		printf("Libv4l2 didn't accept the suggested pixel format. Can't proceed!\n");
        printf("Current format fourcc:  %c%c%c%c\n",
        fmt.fmt.pix.pixelformat,
        fmt.fmt.pix.pixelformat >> 8,
        fmt.fmt.pix.pixelformat >> 16,
        fmt.fmt.pix.pixelformat >> 24);
		return false;
	}
	if((fmt.fmt.pix.width != IMAGE_WIDTH) || (fmt.fmt.pix.height != IMAGE_HEIGHT))
	{
		printf("Warning: driver is sending image at %dx%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);
        return false;
	}

	printf("Device accepted fourcc:  %c%c%c%c\n",  /// RX24==BGRX32
	fmt.fmt.pix.pixelformat,
	fmt.fmt.pix.pixelformat >> 8,
	fmt.fmt.pix.pixelformat >> 16,
	fmt.fmt.pix.pixelformat >> 24);
	printf("Device accepted resolution:  %dx%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);

    m_fmt = fmt;

    return true;
}

bool CameraPlayer::initBuffers(int fd)
{
    m_buffers.clear();
    struct v4l2_requestbuffers req = {0};
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        printf("Error requesting buffers.\n");
        return false;
    }

    for (int i = 0; i < req.count; i++) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
            printf("Error querying buffers.\n");
            return false;
        }

        Buffer buffer;
        buffer.length = buf.length;

        struct v4l2_exportbuffer expbuf = {0};
        expbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        expbuf.index = i;

        if (ioctl(fd, VIDIOC_EXPBUF, &expbuf) == -1) {
            printf("Error exporting buffers.\n");
            return false;
        }

        buffer.fd = expbuf.fd;
        m_buffers.push_back(buffer);
    }

    return true;
}

bool CameraPlayer::queueBuffer(int fd, int index)
{
    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = index;

    if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
        printf("Error queueing buffer.\n");
        return false;
    }

    return true;
}

int CameraPlayer::dequeueBuffer(int fd)
{
    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
        printf("Error dequeueing buffer.\n");
        return -1;
    }

    return buf.index;
}

void CameraPlayer::loadShaders()
{
    m_shader.load("shaders/pass.vert", "shaders/camera.frag");
}

void CameraPlayer::run()
{   
    int fd = open("/dev/video0", O_RDWR);
    if (fd == -1) {
        printf("Error opening video device.\n");
        return;
    }
    m_fd = fd;

    // Set the pixel format for V4L2
    if (!setFormat(fd)) {
        ::close(fd);
        return;
    } 

    // Intialize and export the buffers
    if (!initBuffers(fd)) {
        ::close(fd);
        return;
    }

    printf("CameraPlayer run(): buffer init ok.\n");
    
    // Queue all buffers
    for (int i = 0; i < m_buffers.size(); i++) {
        if (!queueBuffer(fd, i)) return;
    }

    printf("CameraPlayer run(): queue all buffers ok.\n");

    v4l2_buf_type buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(m_fd, VIDIOC_STREAMON, &buf_type))
    {
        perror("VIDIOC_STREAMON");
        finalize();
        return;
    }

	printf("Camera streaming turned ON\n");

    m_isRunning = true;
    while (m_isRunning) {
        VideoFrame frame;
        lockBuffer();
        Buffer* buffer = getBuffer();
        if (buffer) {
            int fd = getBuffer()->fd;
            frame.fds.push_back(fd);
            unlockBuffer();
            m_videoQueue.pushFrame(frame);
        }
        SDL_Delay(10);
    }

    printf("Camera streaming turned OFF\n");

    finalize();

    return;
}

void CameraPlayer::render()
{
    glViewport(0, 0, 1920, 1080);
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_shader.activate();

    glBindVertexArray(m_vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_yuvTextures[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Bind the texture to unit
    GLHelper::glEGLImageTargetTexture2DOESFunc(GL_TEXTURE_2D, m_yuvImages[0]);
    m_shader.bindUniformLocation("inputTexture", 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    m_shader.deactivate();

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CameraPlayer::update()
{
    EGLDisplay display = eglGetCurrentDisplay();

    // Wait for fence and delete it
    eglClientWaitSync(display, m_fence, EGL_SYNC_FLUSH_COMMANDS_BIT, EGL_FOREVER);
    eglDestroySync(display, m_fence);
    m_fence = EGL_NO_SYNC;

    VideoFrame frame;
    if (m_videoQueue.popFrame(frame)) {
        if (m_yuvImages.size() > 0 && frame.fds.size() > 0) {
            
            // Create image
            EGLAttrib img_attr[] = {
                EGL_WIDTH, m_fmt.fmt.pix.width / 2,
                EGL_HEIGHT, m_fmt.fmt.pix.height,
                EGL_LINUX_DRM_FOURCC_EXT, DRM_FORMAT_ARGB8888,
                EGL_DMA_BUF_PLANE0_FD_EXT, frame.fds[0],
                EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
                EGL_DMA_BUF_PLANE0_PITCH_EXT, m_fmt.fmt.pix.bytesperline,
                EGL_NONE
            };
            
            EGLImageKHR image = eglCreateImage(	  
                                    eglGetCurrentDisplay(),
                                    EGL_NO_CONTEXT,
                                    EGL_LINUX_DMA_BUF_EXT,
                                    NULL,
                                    img_attr
                                );

            if(image == EGL_NO_IMAGE_KHR)
            {
                printf("error: eglCreateImageKHR failed: %d\n", eglGetError());
                return false;
            }
            m_yuvImages[0] = image;
        }
    }

    render();

    // Create fence for current frame
    m_fence = eglCreateSync(display, EGL_SYNC_FENCE, NULL);
}
