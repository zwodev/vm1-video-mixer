/*
 * Copyright (c) 2023-2024 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/LICENSE
 * for full license details.
 */

#include "Shader.h"

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <SDL3/SDL.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengles2.h>

static size_t fileGetLength(FILE *file) {
	
	size_t length;
	size_t currPos = ftell(file);
	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, currPos, SEEK_SET);
	
	return length;
}

Shader::Shader()
{
	
}

Shader::~Shader()
{
	if (m_shaderProgram > 0) {
		glDeleteProgram(m_shaderProgram);
	}
}

bool Shader::load(const char *vertFilename, const char *fragFilename){
	
	GLuint vertShader = loadShaderByType(vertFilename, GL_VERTEX_SHADER);
	if(!vertShader) {
		SDL_Log("Couldn't load vertex shader: %s\n", vertFilename);
		return false;
	}
	
	GLuint fragShader = loadShaderByType(fragFilename, GL_FRAGMENT_SHADER);
	if(!fragShader) {
		SDL_Log("Couldn't load fragent shader: %s\n", fragFilename);
		return false;
	}
	
	m_shaderProgram = glCreateProgram();
	if (!m_shaderProgram) {
		SDL_Log("Couldn't create shader program\n");
	}

	glAttachShader(m_shaderProgram, vertShader);
	glAttachShader(m_shaderProgram, fragShader);
	glLinkProgram(m_shaderProgram);
	
	GLint linkingSucceeded = GL_FALSE;
	glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &linkingSucceeded);
	if (!linkingSucceeded) {
		SDL_Log("Linking shader failed (vert. shader: %s, frag. shader: %s\n",
			vertFilename, fragFilename);
		GLint logLength = 0;
		glGetProgramiv(m_shaderProgram, GL_INFO_LOG_LENGTH, &logLength);
		GLchar *errLog = (GLchar*)malloc(logLength);
		if(errLog) {
			glGetProgramInfoLog(m_shaderProgram, logLength, &logLength, errLog);
			SDL_Log("%s\n", errLog);
			free(errLog);
		}
		else {
			SDL_Log("Couldn't get shader link log; out of memory\n");
		}
		glDeleteProgram(m_shaderProgram);
		m_shaderProgram = 0;
		return false;
	}

	glBindAttribLocation(m_shaderProgram, 0, "in_Position");
	glBindAttribLocation(m_shaderProgram, 1, "in_TexCoord");

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	return true;
}

GLuint Shader::loadShaderByType(const char *filename, GLenum shaderType) 
{
	GLuint shader;
	FILE *file = fopen(filename, "r");
	if(!file) {
		SDL_Log("Can't open file: %s\n", filename);
		return 0;
	}
	
	size_t length = fileGetLength(file);
	
	// Alloc space for the file (plus '\0' termination)
	GLchar *shaderSrc = (GLchar*)calloc(length + 1, 1);
	if(!shaderSrc){
		SDL_Log("Out of memory when reading file: %s\n", filename);
		fclose(file);
		file = NULL;
		
		return 0;
	}
	
	fread(shaderSrc, 1, length, file);
	
	// Done with the file
	fclose(file);
	file = nullptr;
	
	// Create the shader
	shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, (const GLchar**)&shaderSrc, NULL);
	free(shaderSrc);
	shaderSrc = NULL;
	
	// Compile it
	glCompileShader(shader);
	GLint compileSucceeded = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileSucceeded);
	
	if(!compileSucceeded) {
		SDL_Log("Compilation of shader %s failed:\n", filename);
		GLint logLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		GLchar *errLog = (GLchar*)malloc(logLength);
		if(errLog) {
			glGetShaderInfoLog(shader, logLength, &logLength, errLog);
			SDL_Log("%s\n", errLog);
			free(errLog);
		}
		else {
			SDL_Log("Couldn't get shader log; out of memory\n");
		}
		
		glDeleteShader(shader);
		shader = 0;
	}
	
	return shader;
}

bool Shader::bindUniformLocation(const char* locName, GLint unit)
{
	GLint texSamplerUniformLoc = glGetUniformLocation(m_shaderProgram, "texSampler");
	if (texSamplerUniformLoc < 0) {
		SDL_Log("ERROR: Couldn't get uniform location with this name: %s", locName);
		return false;
	}
	glUniform1i(texSamplerUniformLoc, unit);
}

void Shader::activate()
{
	glUseProgram(m_shaderProgram);
}

void Shader::deactivate()
{
	glUseProgram(0);
}




