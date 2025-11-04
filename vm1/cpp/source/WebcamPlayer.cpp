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

#include "WebcamPlayer.h"
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


WebcamPlayer::WebcamPlayer()
{
    loadShaders();
    createVertexBuffers();
    initializeFramebufferAndTextures();
}

WebcamPlayer::~WebcamPlayer()
{
    close();  
}

bool WebcamPlayer::openFile(const std::string& fileName, AudioStream* audioStream)
{

}

void WebcamPlayer::close()
{
    finalize();
    MediaPlayer::close();
}

void WebcamPlayer::finalize()
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

void WebcamPlayer::lockBuffer()
{
    if (!m_isRunning) return;    
    m_bufferIndex = dequeueBuffer(m_fd);
}

Buffer* WebcamPlayer::getBuffer()
{
    Buffer* buffer = nullptr;
    if (!m_isRunning) return buffer;    

    if (m_bufferIndex >= 0) {
        buffer = &(m_buffers[m_bufferIndex]);
    }

    return buffer;
}
    
void WebcamPlayer::unlockBuffer()
{
    if (!m_isRunning) return;

    if (m_bufferIndex >= 0) {
        //printf("Enqueue! \n");
        queueBuffer(m_fd, m_bufferIndex);
        m_bufferIndex = -1;
    }
}

bool WebcamPlayer::setFormat(int fd)
{
    struct v4l2_format fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = IMAGE_WIDTH;
	fmt.fmt.pix.height      = IMAGE_HEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field       = V4L2_FIELD_NONE;
	
    // Try setting this format
	ioctl(fd, VIDIOC_S_FMT, &fmt);

	// Check what was actually set
	if(fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV)
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

bool WebcamPlayer::initBuffers(int fd)
{
    m_buffers.clear();
    struct v4l2_requestbuffers req = {0};
    req.count = 3;
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

bool WebcamPlayer::queueBuffer(int fd, int index)
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

int WebcamPlayer::dequeueBuffer(int fd)
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

void WebcamPlayer::loadShaders()
{
    m_shader.load("shaders/pass.vert", "shaders/webcam.frag");
}

static std::vector<CameraMode> listCameraModes(int fd) 
{
    std::vector<CameraMode> modes;

    // Enumerate pixel formats
    struct v4l2_fmtdesc fmtdesc{};
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    for (fmtdesc.index = 0;; ++fmtdesc.index) {
        if (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) < 0) {
            if (errno == EINVAL) break;
            printf("VIDIOC_ENUM_FMT failed\n");
        }

        // Enumerate frame sizes for this pixel format
        struct v4l2_frmsizeenum frmsize{};
        frmsize.pixel_format = fmtdesc.pixelformat;
        for (frmsize.index = 0;; ++frmsize.index) {
            if (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) < 0) {
                if (errno == EINVAL) break;
                printf("VIDIOC_ENUM_FRAMESIZES failed\n");
            }

            if (frmsize.type != V4L2_FRMSIZE_TYPE_DISCRETE)
                continue; // Skip non-discrete

            // Enumerate frame intervals for this pixel format and size
            struct v4l2_frmivalenum frmival{};
            frmival.pixel_format = fmtdesc.pixelformat;
            frmival.width = frmsize.discrete.width;
            frmival.height = frmsize.discrete.height;

            for (frmival.index = 0;; ++frmival.index) {
                if (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) < 0) {
                    if (errno == EINVAL) break;
                    printf("VIDIOC_ENUM_FRAMEINTERVALS failed\n");
                }

                if (frmival.type != V4L2_FRMIVAL_TYPE_DISCRETE)
                    continue; // Skip non-discrete intervals

                modes.push_back(CameraMode{
                    fmtdesc.pixelformat,
                    frmsize.discrete.width,
                    frmsize.discrete.height,
                    frmival.discrete.numerator,
                    frmival.discrete.denominator
                });
            }
        }
    }
    return modes;
}

static std::vector<CameraMode> filterModes(
    const std::vector<CameraMode> &modes,
    __u32 req_width,
    __u32 req_height,
    __u32 req_pixfmt,
    float req_fps)
{
    // Filter only modes with exact resolution and pixel format match
    std::vector<CameraMode> candidates;
    printf("Requested: %dx%d, %d, %f\n", req_width, req_height, req_pixfmt, req_fps);
    for (const auto &m : modes) {
        float fps = static_cast<float>(m.interval_den) / m.interval_num;
        printf("Found: %dx%d, %d, %f\n", m.width, m.height, m.pixelformat, fps);
        if (m.width == req_width && m.height == req_height && m.pixelformat == req_pixfmt) {
            candidates.push_back(m);
        }
    }

    if (candidates.empty()) {
        // No modes with requested resolution and format
        return {};
    }

    // Calculate fps for each candidate and find minimal fps diff from requested fps
    float min_diff = std::numeric_limits<float>::max();
    for (const auto &m : candidates) {
        float fps = static_cast<float>(m.interval_den) / m.interval_num;
        float diff = std::fabs(fps - req_fps);
        if (diff < min_diff) {
            min_diff = diff;
        }
    }
    printf("Min diff: %f\n", min_diff);

    // Return all modes whose fps difference to requested fps equals minimal diff (closest match)
    std::vector<CameraMode> filtered;
    for (const auto &m : candidates) {
        float fps = static_cast<float>(m.interval_den) / m.interval_num;
        float diff = std::fabs(fps - req_fps);
        printf("Diff: %f\n", diff);
        if (diff <= min_diff) {
            filtered.push_back(m);
        }
    }
    return filtered;
}

// Set camera mode according to unified CameraMode struct
bool WebcamPlayer::setCameraMode(int fd, const CameraMode &mode)
{
    v4l2_format fmt{};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = mode.pixelformat;
    fmt.fmt.pix.width = mode.width;
    fmt.fmt.pix.height = mode.height;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        printf("Failed to set pixel format and resolution.\n");
        return false;
    }


    printf("Field: %d", fmt.fmt.pix.field);
    printf("Set format to %ux%u 4cc %c%c%c%c\n",
           fmt.fmt.pix.width, fmt.fmt.pix.height,
           fmt.fmt.pix.pixelformat & 0xFF,
           (fmt.fmt.pix.pixelformat >> 8) & 0xFF,
           (fmt.fmt.pix.pixelformat >> 16) & 0xFF,
           (fmt.fmt.pix.pixelformat >> 24) & 0xFF);

    v4l2_streamparm parm{};
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(fd, VIDIOC_G_PARM, &parm) < 0) {
        printf("Failed to get stream parameters.\n");
        return false;
    }

    parm.parm.capture.timeperframe.numerator = mode.interval_num;
    parm.parm.capture.timeperframe.denominator = mode.interval_den;

    if (ioctl(fd, VIDIOC_S_PARM, &parm) < 0) {
        printf("Failed to set framerate.\n");
        return false;
    }

    printf("Set framerate to %u/%u (%.2f FPS)\n",
           parm.parm.capture.timeperframe.numerator,
           parm.parm.capture.timeperframe.denominator,
           (float)parm.parm.capture.timeperframe.denominator /
           parm.parm.capture.timeperframe.numerator);

    m_fmt = fmt;

    return true;
}

void WebcamPlayer::run()
{   
    int fd = open("/dev/video0", O_RDWR);
    if (fd == -1) {
        printf("Error opening video device.\n");
        return;
    }
    m_fd = fd;

    auto availableFormats = listCameraModes(fd);
    printf("Number of formats: %d\n", availableFormats.size());
    auto filteredFormats = filterModes(availableFormats, 1920, 1080, V4L2_PIX_FMT_YUYV, 60);
    printf("Number of filtered formats: %d\n", filteredFormats.size());
    if(filteredFormats.size() <= 0) {
        printf("Could not find suitable capture mode.\n");
        ::close(fd);
        return;
    }

    if (!setCameraMode(fd, filteredFormats[0])) {
        ::close(fd);
        return;
    }

    // // Set the pixel format for V4L2
    // if (!setFormat(fd)) {[0];
    //     ::close(fd);
    //     return;
    // } 

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
        SDL_Delay(1);
    }

    printf("Camera streaming turned OFF\n");

    finalize();

    return;
}

void WebcamPlayer::render()
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

void WebcamPlayer::update()
{
    EGLDisplay display = eglGetCurrentDisplay();

    // Wait for fence and delete it
    eglClientWaitSync(display, m_fence, EGL_SYNC_FLUSH_COMMANDS_BIT, EGL_FOREVER);
    eglDestroySync(display, m_fence);
    m_fence = EGL_NO_SYNC;

    VideoFrame frame;
    if (m_videoQueue.popFrame(frame)) {
        if (m_yuvImages.size() > 0 && frame.fds.size() > 0) {
            
            // printf("width: %d\n", m_fmt.fmt.pix.width);
            // printf("height: %d\n", m_fmt.fmt.pix.height);
            // printf("fd: %d\n", frame.fds[0]);
            // printf("pitch: %d\n", m_fmt.fmt.pix.bytesperline);

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
