#include "PlaneRenderer.h"
//#include "GLHelper.h"
#include <iostream>

#include <SDL3/SDL.h>

static bool has_EGL_EXT_image_dma_buf_import;
static PFNGLACTIVETEXTUREARBPROC glActiveTextureARBFunc;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOESFunc;
void NewGlInit()
{
    const char *extensions = eglQueryString(eglGetCurrentDisplay(), EGL_EXTENSIONS);
    if (SDL_strstr(extensions, "EGL_EXT_image_dma_buf_import") != NULL) {
        has_EGL_EXT_image_dma_buf_import = SDL_TRUE;
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
	glDeleteTextures(1, &m_texture);
}

bool PlaneRenderer::initialize()
{
	if (!m_shader.load("shaders/simple2D.vert", "shaders/simple2D.frag"))
        return false;

	glGenTextures(1, &m_texture);

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
	//std::cout << "Update" << std::endl;
	m_shader.activate();
	glActiveTextureARBFunc(GL_TEXTURE0_ARB);
	glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Bind the texture to unit 0
    glEGLImageTargetTexture2DOESFunc(GL_TEXTURE_2D, image);
	m_shader.bindUniformLocation("texSampler", 0);
	
    glBindVertexArray(m_vao);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    m_shader.deactivate();
	glBindTexture(GL_TEXTURE_2D, 0);
}

void PlaneRenderer::update(SDL_Texture* texture)
{	
	m_shader.activate();

	// Bind the texture to unit 0
	glActiveTextureARBFunc(GL_TEXTURE0_ARB);
	//glActiveTexture(GL_TEXTURE0);
	SDL_GL_BindTexture(texture, nullptr, nullptr);	
	m_shader.bindUniformLocation("texSampler", 0);
	
    glBindVertexArray(m_vao);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 3);
    glBindVertexArray(0);

    m_shader.deactivate();
}

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