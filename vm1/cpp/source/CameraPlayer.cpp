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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
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
    m_isRunning = false;
}

CameraPlayer::~CameraPlayer()
{   
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

        // Create image
        EGLAttrib img_attr[] = {
            EGL_WIDTH, m_fmt.fmt.pix.width / 2,
            EGL_HEIGHT, m_fmt.fmt.pix.height,
            EGL_LINUX_DRM_FOURCC_EXT, DRM_FORMAT_ARGB8888,
            EGL_DMA_BUF_PLANE0_FD_EXT, expbuf.fd,
            EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
            EGL_DMA_BUF_PLANE0_PITCH_EXT, m_fmt.fmt.pix.bytesperline,
            //EGL_DMA_BUF_PLANE0_PITCH_EXT, m_fmt.fmt.pix.width,
            EGL_NONE
        };
        EGLImageKHR image = eglCreateImage(	  
                                eglGetCurrentDisplay(),
                                EGL_NO_CONTEXT,
                                EGL_LINUX_DMA_BUF_EXT,
                                NULL,
                                img_attr
                            );
        printf("Pitch: %d\n", m_fmt.fmt.pix.bytesperline);
        if(image == EGL_NO_IMAGE_KHR)
        {
            printf("error: eglCreateImageKHR failed: %d\n", eglGetError());
            return false;
        }

        buffer.image = image;
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

bool CameraPlayer::start()
{
    if (m_isRunning) return;
    
    int fd = open("/dev/video0", O_RDWR);
    if (fd == -1) {
        printf("Error opening video device.\n");
        return false;
    }
    m_fd = fd;

    // Set the pixel format for V4L2
    if (!setFormat(fd)) return false;

    // Intialize and export the buffers
    if (!initBuffers(fd)) return false;

    // Queue all buffers
    for (int i = 0; i < m_buffers.size(); i++) {
        if (!queueBuffer(fd, i)) return false;
    }

    v4l2_buf_type buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(fd, VIDIOC_STREAMON, &buf_type))
    {
        perror("VIDIOC_STREAMON");
        return -1;
    }

	printf("Camera streaming turned ON\n");

    m_isRunning = true;
}
