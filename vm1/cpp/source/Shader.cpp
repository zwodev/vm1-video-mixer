/*
 * Copyright (c) 2023-2024 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include "Shader.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <regex>

#include <SDL3/SDL.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengles2.h>
#include <GLES3/gl31.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

static size_t fileGetLength(FILE* file) {
	
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

void fetchIntParameterFromJson(const std::string& name, IntParameter& param, const json& jsonData) {
	if (jsonData.contains(name)) {
		const json& value = jsonData[name];
		if (value.is_number()) {
			if (name == "default") param.value = value.get<int>();
			if (name == "min") param.min = value.get<int>();
			if (name == "max") param.max = value.get<int>();
			if (name == "step") param.step = value.get<int>();
		}
	}
}

void fetchFloatParameterFromJson(const std::string& name, FloatParameter& param, const json& jsonData) {
	if (jsonData.contains(name)) {
		const json& value = jsonData[name];
		if (value.is_number()) {
			if (name == "default") param.value = value.get<float>();
			if (name == "min") param.min = value.get<float>();
			if (name == "max") param.max = value.get<float>();
			if (name == "step") param.step = value.get<float>();
		}
	}
}

void fetchVec2ParameterFromJson(const std::string& name, Vec2Parameter& param, const json& jsonData) {
	if (jsonData.contains(name)) {
		const json& vec2Array = jsonData[name];
		if (vec2Array.is_array() && vec2Array.size() == 2) {
			Vec2 vec;
			for (int i = 0; i < vec2Array.size(); ++i) {
				const json& value = vec2Array[i];
				if (value.is_number_float()) {
					vec[i] = value.get<float>();
				}
			}

			if (name == "default") param.value = vec;
			if (name == "min") param.min = vec;
			if (name == "max") param.max = vec;
			if (name == "step") param.step = vec;
		}
	}
}

void Shader::parseUnifromJson(const std::string& uniformName, const std::string& uniformType, const std::string& uniformJson)
{
	json jsonData = json::parse(uniformJson);
	if (uniformType == "int") {
		auto& param = m_shaderConfig.params[uniformName];
		if (std::holds_alternative<IntParameter>(param)) {
			IntParameter& intParam = std::get<IntParameter>(param);  
			fetchIntParameterFromJson("default", intParam, jsonData);
			fetchIntParameterFromJson("min", intParam, jsonData);
			fetchIntParameterFromJson("max", intParam, jsonData);
			fetchIntParameterFromJson("step", intParam, jsonData);

		}
	}
	else if (uniformType == "uint") {
		// TODO: implement UIntParameter
	}
	else if (uniformType == "float") {
		auto& param = m_shaderConfig.params[uniformName];
		if (std::holds_alternative<FloatParameter>(param)) {
			FloatParameter& floatParam = std::get<FloatParameter>(param);  
			fetchFloatParameterFromJson("default", floatParam, jsonData);
			fetchFloatParameterFromJson("min", floatParam, jsonData);
			fetchFloatParameterFromJson("max", floatParam, jsonData);
			fetchFloatParameterFromJson("step", floatParam, jsonData);
		}
	}
	else if (uniformType == "vec2") {
		auto& param = m_shaderConfig.params[uniformName];
		if (std::holds_alternative<FloatParameter>(param)) {
			Vec2Parameter& vec2Param = std::get<Vec2Parameter>(param);  
			fetchVec2ParameterFromJson("default", vec2Param, jsonData);
			fetchVec2ParameterFromJson("min", vec2Param, jsonData);
			fetchVec2ParameterFromJson("max", vec2Param, jsonData);
			fetchVec2ParameterFromJson("step", vec2Param, jsonData);
		}
	}
}

void Shader::extractUniformMetadata(const char* filename) 
{
	static std::regex uniform_regex(R"(\s*uniform\s+(\w+)\s+(\w+)\s*;\s*//\s*(\{.*\}))", std::regex::multiline);

	std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open: " << filename << std::endl;
		return;
    }
    
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string shaderCode(buffer.str());
	file.close();
    
	std::sregex_iterator next(shaderCode.begin(), shaderCode.end(), uniform_regex);
  	std::sregex_iterator end;

	try {
		while (next != end) {
			std::smatch match = *next;
			if (match.size() >= 3) {
				std::string uniformType = match[1].str();
				std::string uniformName = match[2].str();
				std::string uniformJson = match[3].str();
				printf("Uniform: %s, Type: %s, JSON: %s\n", uniformName.c_str(), uniformType.c_str(), uniformJson.c_str());
				
				next++;
			}
		}
	} catch (std::regex_error& e) {
  		// Syntax error in the regular expression
	}

	//auto begin = shaderCode.cbegin();
    //auto end = shaderCode.cend();
    // while (std::regex_search(begin, end, matches, uniform_regex)) {
	// 	if (matches.size() >= 3) {
    //     	std::string uniformName = matches[1].str();
    //     	std::string uniformJson = matches[2].str();
	// 		//printf("Uniform: %s, JSON: %s\n", uniformName.c_str(), uniformJson.c_str());
	// 	}
    //     begin = matches.suffix().first;
    // }
}

void Shader::createShaderConfigFromUniforms()
{
	GLint uniformCount;
	glGetProgramiv(m_shaderProgram, GL_ACTIVE_UNIFORMS, &uniformCount);
	for(GLint i = 0; i < uniformCount; i++) {
		GLint size, length;
		GLenum type;
		GLchar name[256];
		glGetActiveUniform(m_shaderProgram, i, 256, &length, &size, &type, name);

		if (size != 1) {
			SDL_Log("Uniforms of size > 0 (arrays/structs) are not supported.\n");
			continue;
		}

		switch (type) {
			case GL_FLOAT:
				m_shaderConfig.params[name] = FloatParameter(std::string(name), 0.0f);
				break;
			case GL_FLOAT_VEC2:
				m_shaderConfig.params[name] = Vec2Parameter(std::string(name), Vec2(0.0f, 0.0f));
				break;
			case GL_INT:
				m_shaderConfig.params[name] = IntParameter(std::string(name), 0);
				break;
			default:
				SDL_Log("This uniform type is not supported: %d\n", type);
		}
	}
}

// std::vector<Parameters> Shader::getUniforms()
// {
// 	// return vector
// }

bool Shader::link()
{
	glLinkProgram(m_shaderProgram);
	GLint linkingSucceeded = GL_FALSE;
	glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &linkingSucceeded);
	if (!linkingSucceeded) {
		//SDL_Log("Linking shader failed (vert. shader: %s, frag. shader: %s\n", vertFilename, fragFilename);
		SDL_Log("Linking shader failed!");
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

	return true;
}

bool Shader::load(const char* vertFilename, const char* fragFilename){
	
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
	if (link()) {
		activate();
		createShaderConfigFromUniforms();
		extractUniformMetadata(fragFilename);
		deactivate();
	}
	glBindAttribLocation(m_shaderProgram, 0, "in_Position");
	glBindAttribLocation(m_shaderProgram, 1, "in_TexCoord");

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	return true;
}

bool Shader::load(const char* compFilename)
{
	GLuint compShader = loadShaderByType(compFilename, GL_COMPUTE_SHADER);
	if(!compShader) {
		SDL_Log("Couldn't load compute shader: %s\n", compFilename);
		return false;
	}
	
	m_shaderProgram = glCreateProgram();
	if (!m_shaderProgram) {
		SDL_Log("Couldn't create compute shader program\n");
	}

	glAttachShader(m_shaderProgram, compShader);
	link();

	glDeleteShader(compShader);

	return true;
}

GLuint Shader::loadShaderByType(const char* filename, GLenum shaderType) 
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
	GLint texSamplerUniformLoc = glGetUniformLocation(m_shaderProgram, locName);
	if (texSamplerUniformLoc < 0) {
		SDL_Log("ERROR: Couldn't get uniform location with this name: %s", locName);
		return false;
	}
	glUniform1i(texSamplerUniformLoc, unit);
	return true;
}

bool Shader::setValue(const char* locName, GLfloat value)
{
	GLint uniformLoc = glGetUniformLocation(m_shaderProgram, locName);
	if (uniformLoc < 0) {
		SDL_Log("ERROR: Couldn't get uniform location with this name: %s", locName);
		return false;
	}
	glUniform1f(uniformLoc, value);
	return true;
}

bool Shader::setValue(const char* locName, GLint value)
{
	GLint uniformLoc = glGetUniformLocation(m_shaderProgram, locName);
	if (uniformLoc < 0) {
		SDL_Log("ERROR: Couldn't get uniform location with this name: %s", locName);
		return false;
	}
	glUniform1i(uniformLoc, value);
	return true;
}

void Shader::activate()
{
	glUseProgram(m_shaderProgram);
}

void Shader::deactivate()
{
	glUseProgram(0);
}

const ShaderConfig& Shader::shaderConfig()
{
	return m_shaderConfig;
}


