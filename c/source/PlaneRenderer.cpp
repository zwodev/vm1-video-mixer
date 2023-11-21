#include "PlaneRenderer.h"

#include <SDL3/SDL.h>

//Create the triangle
const VertexWithTex vertices[] = {
    {{-0.9f, -0.9f}, {0.0f, 0.0f}},
    {{0.9f, -0.9f}, {1.0f, 0.0f}},
    {{0.9f, 0.9f}, {1.0f, 1.0f}},
    {{-0.9f, 0.9f}, {0.0f, 1.0f}}};

PlaneRenderer::PlaneRenderer()
{
    initialize();
}

PlaneRenderer::~PlaneRenderer()
{
}

bool PlaneRenderer::initialize()
{
	if (!m_shader.load("shaders/simple2D.vert", "shaders/simple2D.frag"))
        return false;

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

void PlaneRenderer::update()
{	
	m_shader.activate();

	// // Bind the texture to unit 0
	// glActiveTexture(GL_TEXTURE0);
	// glBindTexture(GL_TEXTURE_2D, m_texture);
	
	// // Bind texSampler to unit 0
	// GLint texSamplerUniformLoc = glGetUniformLocation(shaderProg, "texSampler");
	// if (texSamplerUniformLoc < 0) {
	// 	SDL_Log("ERROR: Couldn't get texSampler's location.");
	// 	return false
	// }
	// glUniform1i(texSamplerUniformLoc, 0);
	
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