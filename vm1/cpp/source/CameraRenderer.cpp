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

#include "CameraRenderer.h"

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

static bool has_EGL_EXT_image_dma_buf_import;
static PFNGLACTIVETEXTUREARBPROC glActiveTextureARBFunc;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOESFunc;
void NewNewGlInit()
{
    const char *extensions = eglQueryString(eglGetCurrentDisplay(), EGL_EXTENSIONS);
    if (SDL_strstr(extensions, "EGL_EXT_image_dma_buf_import") != NULL) {
        has_EGL_EXT_image_dma_buf_import = true;
    }

    if (SDL_GL_ExtensionSupported("GL_OES_EGL_image")) {
        glEGLImageTargetTexture2DOESFunc = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
    }

    glActiveTextureARBFunc = (PFNGLACTIVETEXTUREARBPROC)SDL_GL_GetProcAddress("glActiveTextureARB");
}

using namespace std;

const int IMAGE_WIDTH = 1920;
const int IMAGE_HEIGHT = 1080;


CameraRenderer::CameraRenderer()
{
    NewNewGlInit();
    initialize();
}

CameraRenderer::~CameraRenderer()
{   

    glDeleteTextures(1, &m_rgbTexture);
}

void CameraRenderer::createGeometryBuffers()
{   
    int cols = 1;
    float stepX = 1.0f / 1.0f;
    for (int i = 0; i < cols; ++i) {
        float offset = float(cols * 102) / float(1530);
        float startX = float(i) * stepX;
        float endX = float(i+1) * stepX;
        VertexWithTex v0(startX, 0.0f, 0.0f, 1.0f, offset);
        m_vertices.push_back(v0);    
        VertexWithTex v1(endX, 0.0f, 1.0f, 1.0f, offset);
        m_vertices.push_back(v1);
        VertexWithTex v2(endX, 1.0f, 1.0f, 0.0f, offset);
        m_vertices.push_back(v2);
        VertexWithTex v3(startX, 1.0f, 0.0f, 0.0f, offset);
        m_vertices.push_back(v3);

        m_indices.push_back(i*4+0);
        m_indices.push_back(i*4+2);
        m_indices.push_back(i*4+1);
        m_indices.push_back(i*4+0);
        m_indices.push_back(i*4+3);
        m_indices.push_back(i*4+2);
    }
}

bool CameraRenderer::initialize()
{
	if (!m_shader.load("shaders/pass.vert", "shaders/camera.frag"))
        return false;

    createGeometryBuffers();
	if (!createVbo() || !createIbo())
        return false;

    glGenVertexArrays(1, &m_vao);
    if (!m_vao) {
        GLenum err = glGetError();
        SDL_Log("Could not create Vertex Array Object: %u\n", err);
        return false;
    }

    glBindVertexArray(m_vao);
    
    GLuint positionIdx = 0; // Position is vertex attribute 0
    glEnableVertexAttribArray(positionIdx);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glVertexAttribPointer(positionIdx, 2, GL_FLOAT, GL_FALSE, sizeof(VertexWithTex), (const GLvoid *) 0);
	
	GLuint texCoordIdx = 1; // TexCoord is vertex attribute 1
    glEnableVertexAttribArray(texCoordIdx);
	glVertexAttribPointer(texCoordIdx, 2, GL_FLOAT, GL_FALSE, sizeof(VertexWithTex), (const GLvoid*) (sizeof(float) * 2));

    GLuint offsetIdx = 2; // Offset is vertex attribute 2
    glEnableVertexAttribArray(offsetIdx);
	glVertexAttribPointer(offsetIdx, 1, GL_FLOAT, GL_FALSE, sizeof(VertexWithTex), (const GLvoid*) (sizeof(float) * 4));

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_ibo);
   
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

bool CameraRenderer::setFormat(int fd)
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

bool CameraRenderer::initBuffers(int fd)
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
            EGL_WIDTH, m_fmt.fmt.pix.width,
            EGL_HEIGHT, m_fmt.fmt.pix.height,
            EGL_LINUX_DRM_FOURCC_EXT, DRM_FORMAT_UYVY,
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

bool CameraRenderer::queueBuffer(int fd, int index)
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

int CameraRenderer::dequeueBuffer(int fd)
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

bool CameraRenderer::start()
{
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

    // Create textures
	glGenTextures(1, &m_rgbTexture);


    m_isRunning = true;
}

void CameraRenderer::update()
{   
    if (!m_isRunning) return;

    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, m_rgbTexture);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    int index = dequeueBuffer(m_fd);
    if (index >= 0) {
        printf("Dequeue! \n");
        Buffer& buffer = m_buffers[index];
        glEGLImageTargetTexture2DOESFunc(GL_TEXTURE_EXTERNAL_OES, buffer.image);
        queueBuffer(m_fd, index);
    }

    // // RENDER TO SCREEN: Convert YUV to RGB
    // glViewport(0, 0, 1920, 1080);
    // glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT);
    // m_shader.activate();
    // std::string locName("rgbTexture");
    // m_shader.bindUniformLocation(locName.c_str(), 0);

    // glBindVertexArray(m_vao);
    // glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, NULL);
    // glBindVertexArray(0);
    // m_shader.deactivate();

    // Unbind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool CameraRenderer::createVbo()
{
    if (m_vbo > 0) glDeleteBuffers(1, &m_vbo);


	glGenBuffers(1, &m_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexWithTex) * m_vertices.size(), &(m_vertices[0]), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	GLenum err = glGetError();
	if(err != GL_NO_ERROR) {
		glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
		SDL_Log("Creating VBO failed, code %u\n", err);
        return false;
	}

    return true;
}

bool CameraRenderer::createIbo()
{
    if (m_ibo > 0) glDeleteBuffers(1, &m_ibo);

	glGenBuffers(1, &m_ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * m_indices.size(), &(m_indices[0]), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	GLenum err = glGetError();
	if(err != GL_NO_ERROR) {
		glDeleteBuffers(1, &m_ibo);
        m_ibo = 0;
		SDL_Log("Creating IBO failed, code %u\n", err);
        return false;
	}

    return true;
}
