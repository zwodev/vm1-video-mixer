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
using namespace std;

const int NUM_TEXTURES = 2;

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

//Create the triangle
const VertexWithTex vertices[] = {
    {{-1.0f, -1.0f}, {0.0f, 1.0f}},
    {{1.0f, -1.0f}, {1.0f, 1.0f}},
    {{1.0f, 1.0f}, {1.0f, 0.0f}},
    {{-1.0f, 1.0f}, {0.0f, 0.0f}}
};

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

bool PlaneRenderer::initialize()
{
	if (!m_shader.load("shaders/simple2D.vert", "shaders/simple2D.frag"))
        return false;

    //createAndFillTexture();

    for (int i = 0; i < NUM_TEXTURES; ++i) {
        GLuint id;
        glGenTextures(1, &id);
        m_yuvTextures.push_back(id);
    }	

	GLsizei vertSize = sizeof(vertices[0]);
	GLsizei numVertices = sizeof(vertices)/vertSize;
	if (!createVboFromVertices(vertices, numVertices))
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
   
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

void PlaneRenderer::update(EGLImage image)
{
	m_shader.activate();

    for (int i = 0; i < m_yuvTextures.size(); ++i) {
        glActiveTextureARBFunc(GL_TEXTURE0_ARB + i);
        glBindTexture(GL_TEXTURE_2D, m_yuvTextures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Bind the texture to unit
        glEGLImageTargetTexture2DOESFunc(GL_TEXTURE_2D, image);
        //std::string locName("texSampler");
        //locName += std::to_string(i);
        //m_shader.bindUniformLocation(locName.c_str(), 0);
    }
	
    glBindVertexArray(m_vao);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    m_shader.deactivate();
	glBindTexture(GL_TEXTURE_2D, 0);
}

void PlaneRenderer::update(std::vector<YUVImage> yuvImages, float mixValue)
{
    for (int i = 0; i < m_yuvTextures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_yuvTextures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Bind the texture to unit
        glEGLImageTargetTexture2DOESFunc(GL_TEXTURE_2D, yuvImages[i].yImage);
    }

    m_shader.activate();
    for (int i = 0; i < m_yuvTextures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_yuvTextures[i]);
        std::string locName("yuvTexture");
        locName += std::to_string(i);
        m_shader.bindUniformLocation(locName.c_str(), i);
    }
    
    m_shader.setValue("mixValue", mixValue);
    //glActiveTexture(GL_TEXTURE0 + m_yuvTextures.size());
    //glBindTexture(GL_TEXTURE_2D, m_coordTexture);
    //m_shader.bindUniformLocation("coordTexture", m_yuvTextures.size());

    glBindVertexArray(m_vao);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    m_shader.deactivate();

    // Unbind textures
    for (int i = 0; i < m_yuvTextures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glActiveTexture(GL_TEXTURE0 + m_yuvTextures.size());
    glBindTexture(GL_TEXTURE_2D, 0);
}


// void PlaneRenderer::update(SDL_Texture* texture)
// {	
// 	m_shader.activate();

// 	// Bind the texture to unit 0
// 	glActiveTextureARBFunc(GL_TEXTURE0_ARB);
// 	//glActiveTexture(GL_TEXTURE0);
// 	//SDL_GL_BindTexture(texture, nullptr, nullptr);	
// 	glBindTexture(texture, nullptr, nullptr);	
// 	m_shader.bindUniformLocation("texSampler", 0);
	
//     glBindVertexArray(m_vao);
// 	glDrawArrays(GL_TRIANGLE_FAN, 0, 3);
//     glBindVertexArray(0);

//     m_shader.deactivate();
// }

bool PlaneRenderer::createVboFromVertices(const VertexWithTex* vertices, GLuint numVertices)
{
    if (m_vbo > 0) glDeleteBuffers(1, &m_vbo);


	glGenBuffers(1, &m_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexWithTex) * numVertices, vertices, GL_STATIC_DRAW);
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

const vec2 IN_TEX_SIZE = vec2(2048.0f, 1530.0f);
const vec2 OUT_TEX_SIZE = vec2(1920.0f, 1080.0f);
const vec2 STEP = vec2(1.0f / IN_TEX_SIZE.x, 1.0f / IN_TEX_SIZE.y);
const vec2 D = vec2(0.5f / IN_TEX_SIZE.x, 0.5f / IN_TEX_SIZE.y);
const float COLUMN_WIDTH = 128.0f;
const float INV_COLUMN_WIDTH = 1.0f / 128.0f;

vec2 getYCoord(vec2 coord)
{
	float x = floorf(coord.x);
	float y = floorf(coord.y);
	float col = floorf(x / COLUMN_WIDTH);
	float posInCol = y * COLUMN_WIDTH + floorf(fmodf(x, COLUMN_WIDTH));
	float xNew = fmodf(posInCol, IN_TEX_SIZE.x);
	float yNew = col * 102.0f + floorf(posInCol / IN_TEX_SIZE.x);
	vec2 uv = vec2((xNew / IN_TEX_SIZE.x) + D.x, (yNew / IN_TEX_SIZE.y) + D.y);
	return uv;
}

vec2 getUVCoord(vec2 coord)
{
	float x = floorf(coord.x);
	float y = floorf(coord.y * 0.5f);
	float col = floorf(x / COLUMN_WIDTH);
	float posInCol = y * COLUMN_WIDTH + floorf(fmodf(x, COLUMN_WIDTH));
	float xNew = floorf(fmodf(posInCol, IN_TEX_SIZE.x) / 2.0f) * 2.0f;
	float yNew = 68.0f + col * 102.0f + floorf(posInCol / IN_TEX_SIZE.x);
	vec2 uv = vec2((xNew / IN_TEX_SIZE.x) + D.x, (yNew / IN_TEX_SIZE.y) + D.y);
	return uv;
}

void PlaneRenderer::createAndFillTexture() 
{
    glGenTextures(1, &m_coordTexture);
    glBindTexture(GL_TEXTURE_2D, m_coordTexture);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create and fill data
    float* data = new float[(int)OUT_TEX_SIZE.x * (int)OUT_TEX_SIZE.y * 4];
    for (int y = 0; y < OUT_TEX_SIZE.y; y++) {
        for (int x = 0; x < OUT_TEX_SIZE.x; x++) {
            vec2 coordY = getYCoord(vec2(x,y));
            vec2 coordUV = getUVCoord(vec2(x,y));
            int index = (y * OUT_TEX_SIZE.x + x) * 4;
            data[index] = (float)coordY.x;
            data[index + 1] = (float)coordY.y;
            data[index + 2] = (float)coordUV.x;
            data[index + 3] = (float)coordUV.y;
            //data[index] = 1.0f;
            //data[index + 1] = 1.0f;
            //data[index + 2] = 1.0f;
            //data[index + 3] = 1.0f;
            // data[index] = 255;
            // data[index + 1] = 255;
            // data[index + 2] = 0;
            // data[index + 3] = 255;
        }
    }

    //std::cout << "Before!!" << std::endl;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (int)OUT_TEX_SIZE.x, (int)OUT_TEX_SIZE.y, 0, GL_RGBA, GL_FLOAT, data);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)OUT_TEX_SIZE.x, (int)OUT_TEX_SIZE.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("OpenGL Error: %d\n", error);
    }
    const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
    printf("Supported Extensions: %s\n", extensions);
    
    glBindTexture(GL_TEXTURE_2D, 0);

    delete[] data;
}

