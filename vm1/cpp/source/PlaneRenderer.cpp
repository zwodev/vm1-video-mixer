/*
 * Copyright (c) 2023-2024 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */


#include "PlaneRenderer.h"
#include <iostream>
#include <math.h>
#include <SDL3/SDL.h>
#include <GLES3/gl31.h>
using namespace std;

const int NUM_TEXTURES = 2;
const int YUV_IMAGE_WIDTH = 2048;
const int YUV_IMAGE_HEIGHT = 1530;

static bool has_EGL_EXT_image_dma_buf_import;
static PFNGLACTIVETEXTUREARBPROC glActiveTextureARBFunc;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOESFunc;
void NewGlInit()
{
    const char *extensions = eglQueryString(eglGetCurrentDisplay(), EGL_EXTENSIONS);
    if (SDL_strstr(extensions, "EGL_EXT_image_dma_buf_import") != NULL) {
        has_EGL_EXT_image_dma_buf_import = true;
    }

    if (SDL_GL_ExtensionSupported("GL_OES_EGL_image")) {
        glEGLImageTargetTexture2DOESFunc = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
    }

    glActiveTextureARBFunc = (PFNGLACTIVETEXTUREARBPROC)SDL_GL_GetProcAddress("glActiveTextureARB");

    // if (has_EGL_EXT_image_dma_buf_import &&
    //     glEGLImageTargetTexture2DOESFunc &&
    //     glActiveTextureARBFunc) {
    //     m_hasEglCreateImage = true;
    // }
}

PlaneRenderer::PlaneRenderer()
{
	NewGlInit();
    initialize();
}

PlaneRenderer::~PlaneRenderer()
{   
    for (int i = 0; i < m_yuvTextures.size(); ++i) {
        GLuint id = m_yuvTextures[i];
        glDeleteTextures(1, &id);
    }

    glDeleteTextures(1, &m_coordTexture);
}

void PlaneRenderer::createGeometryBuffers()
{   
    int cols = 1;
    //float offset = 0.0f;
    //int cols = 1920 / 128;    
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

bool PlaneRenderer::initialize()
{
	if (!m_shader.load("shaders/pass.vert", "shaders/yuv2rgb_sand128_and_camera.frag"))
        return false;

    //if (!m_shader.load("shaders/pass.vert", "shaders/yuv2rgb_sand128.frag"))
    //    return false;

    if (!m_mixShader.load("shaders/pass.vert", "shaders/yuv_mix.frag"))
        return false;

    createGeometryBuffers();
	if (!createVbo() || !createIbo())
        return false;

    // Generate textures for video images
    for (int i = 0; i < NUM_TEXTURES; ++i) {
        GLuint id;
        glGenTextures(1, &id);
        m_yuvTextures.push_back(id);
    }

    // Generate textures for camera images
    for (int i = 0; i < NUM_TEXTURES; ++i) {
        GLuint id;
        glGenTextures(1, &id);
        m_yuyvTextures.push_back(id);
    }

    // Generate and bind the framebuffer
    GLuint fbId;
    glGenFramebuffers(1, &m_frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

    // Create texture
    GLuint texId;
    glGenTextures(1, &m_compTexture);
    glBindTexture(GL_TEXTURE_2D, m_compTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, YUV_IMAGE_WIDTH, YUV_IMAGE_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_compTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);


    for (int i = 0; i < NUM_TEXTURES; ++i) {
        GLuint id;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, YUV_IMAGE_WIDTH, YUV_IMAGE_HEIGHT);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_inputTextures.push_back(id);
    }

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

void PlaneRenderer::update(std::vector<YUVImage> yuvImages, std::vector<YUVImage> yuyvImages, float videoMixValue, float cameraMixValue)
{   
    for (int i = 0; i < yuvImages.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_yuvTextures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Bind the texture to unit
        glEGLImageTargetTexture2DOESFunc(GL_TEXTURE_2D, yuvImages[i].yImage);
    }

    // RENDER TO TEXTURE: Mix YUV images
    glViewport(0, 0, YUV_IMAGE_WIDTH, YUV_IMAGE_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    m_mixShader.activate();
    for (int i = 0; i < m_yuvTextures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_yuvTextures[i]);
        std::string locName("yuvTexture");
        locName += std::to_string(i);
        m_mixShader.bindUniformLocation(locName.c_str(), i);
    } 

    m_mixShader.setValue("mixValue", videoMixValue);  
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, NULL);
    glBindVertexArray(0);
    m_mixShader.deactivate();

    // Unbind textures
    for (int i = 0; i < m_yuvTextures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // RENDER TO SCREEN: Convert YUV to RGB
    glViewport(0, 60, 1920, 1080);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (int i = 0; i < yuyvImages.size(); ++i) {
        int slot = i + 1;
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_yuyvTextures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Bind the texture to unit
        glEGLImageTargetTexture2DOESFunc(GL_TEXTURE_2D, yuyvImages[i].yImage);
    }
    
    m_shader.activate();

    // Bind the mixed YUV (NV12 SAND128) texture from before 
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_compTexture);
    std::string locName("yuvTexture");
    m_shader.bindUniformLocation(locName.c_str(), 0);

    // Bind all camera textures (YUYV)
    for (int i = 0; i < yuyvImages.size(); ++i) {
        int slot = i + 1;
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_yuyvTextures[i]);
        std::string locName("yuyvTexture");
        locName += std::to_string(i);
        m_shader.bindUniformLocation(locName.c_str(), slot);
    }

    // Set mix value
    m_shader.setValue("mixValue", cameraMixValue); 

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, NULL);
    glBindVertexArray(0);
    m_shader.deactivate();

    // Unbind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool PlaneRenderer::createVbo()
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

bool PlaneRenderer::createIbo()
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
