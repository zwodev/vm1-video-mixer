/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include "Shader.h"
#include "StringHelper.h"

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
		if (value.is_number_integer()) {
			if (name == "default") param.value = value.get<int>();
			if (name == "min") param.min = value.get<int>();
			if (name == "max") param.max = value.get<int>();
			if (name == "step") param.step = value.get<int>();
		}
		else if (value.is_string()) {
			if (name == "name") param.name = value.get<std::string>();
		}
	}
}

void fetchFloatParameterFromJson(const std::string& name, FloatParameter& param, const json& jsonData) {
	if (jsonData.contains(name)) {
		const json& value = jsonData[name];
		if (value.is_number_float()) {
			if (name == "default") param.value = value.get<float>();
			if (name == "min") param.min = value.get<float>();
			if (name == "max") param.max = value.get<float>();
			if (name == "step") param.step = value.get<float>();
		}
		else if (value.is_string()) {
			if (name == "name") param.name = value.get<std::string>();
		}
	}
}

void fetchVec2ParameterFromJson(const std::string& name, Vec2Parameter& param, const json& jsonData) {
	if (jsonData.contains(name)) {
		const json& value = jsonData[name];
		if (value.is_array() && value.size() == 2) {
			Vec2 vec;
			for (size_t i = 0; i < value.size(); ++i) {
				const json& component = value[i];
				if (component.is_number_float()) {
					vec[i] = component.get<float>();
				}
			}

			if (name == "default") param.value = vec;
			if (name == "min") param.min = vec;
			if (name == "max") param.max = vec;
			if (name == "step") param.step = vec;
		}
		else if (value.is_string()) {
			if (name == "name") param.name = value.get<std::string>();
		}
	}
}

void Shader::parseUnifromJson(const std::string& uniformName, const std::string& uniformType, const std::string& uniformJson)
{
	if (!m_shaderConfig.params.contains(uniformName)) return;

	static const std::vector<std::string> valueNames = {"name", "default", "min", "max", "step"};

	json jsonData = json::parse(uniformJson);
	if (jsonData.contains("group")) {
		const json& value = jsonData["group"];
		if (value.is_string()) {
			std::string groupName = value.get<std::string>();
			if (!m_shaderConfig.groups.contains(groupName)) {
				m_shaderConfig.groups[groupName] = std::vector<std::string>();
			}
			m_shaderConfig.groups[groupName].push_back(uniformName);
		}
	}

	if (uniformType == "int") {
		auto& param = m_shaderConfig.params[uniformName];
		if (std::holds_alternative<IntParameter>(param)) {
			IntParameter& intParam = std::get<IntParameter>(param);
			for (const auto& valueName : valueNames) {
				fetchIntParameterFromJson(valueName, intParam, jsonData);
			}
		}
	}
	else if (uniformType == "uint") {
		// TODO: implement UIntParameter
	}
	else if (uniformType == "float") {
		auto& param = m_shaderConfig.params[uniformName];
		if (std::holds_alternative<FloatParameter>(param)) {
			FloatParameter& floatParam = std::get<FloatParameter>(param);
			for (const auto& valueName : valueNames) {
				fetchFloatParameterFromJson(valueName, floatParam, jsonData);
			}
		}
	}
	else if (uniformType == "vec2") {
		auto& param = m_shaderConfig.params[uniformName];
		if (std::holds_alternative<Vec2Parameter>(param)) {
			Vec2Parameter& vec2Param = std::get<Vec2Parameter>(param);  
			for (const auto& valueName : valueNames) {
				fetchVec2ParameterFromJson(valueName, vec2Param, jsonData);
			}
		}
	}
}

void Shader::extractUniformMetadata() 
{
	static std::regex uniform_regex(R"(^\s*uniform\s+(\w+)\s+(\w+)\s*;\s*//\s*(\{.*\}))", std::regex::multiline);

	std::sregex_iterator next(m_shaderSrc.begin(), m_shaderSrc.end(), uniform_regex);
  	std::sregex_iterator end;

	//try {
		while (next != end) {
			std::smatch match = *next;
			if (match.size() >= 3) {
				std::string uniformType = match[1].str();
				std::string uniformName = match[2].str();
				std::string uniformJson = match[3].str();
				printf("Uniform: %s, Type: %s, JSON: %s\n", uniformName.c_str(), uniformType.c_str(), uniformJson.c_str());
				parseUnifromJson(uniformName, uniformType, uniformJson);
				next++;
			}
		}
	//} 
	// catch (std::regex_error& e) {
  	// 	// Syntax error in the regular expression
	// }

	for (auto& kv : m_shaderConfig.params) {
		auto& param = kv.second;
		if (std::holds_alternative<IntParameter>(param)) {
			auto& intParam = std::get<IntParameter>(param);  
			printf("Int -> Name: %s, Value: %d, Min: %d, Max: %d, Step: %d\n", intParam.name.c_str(), intParam.value, intParam.min, intParam.max, intParam.step);
		} else if (std::holds_alternative<FloatParameter>(param)) {
			auto& floatParam = std::get<FloatParameter>(param); 
			printf("Float -> Name: %s, Value: %f, Min: %f, Max: %f, Step: %f\n", floatParam.name.c_str(), floatParam.value, floatParam.min, floatParam.max, floatParam.step);
		}
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
			//default:
			//	SDL_Log("This uniform type is not supported: %d\n", type);
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

bool Shader::load(const std::string& vertFilename, const std::string& fragFilename, const std::string& extFilename) 
{
	if (!strhlpr::isFile(vertFilename) || !strhlpr::isFile(fragFilename)) return false;
	
	if (!extFilename.empty() && !strhlpr::isFile(extFilename)) return false;

	GLuint vertShader = loadShaderByType(vertFilename, GL_VERTEX_SHADER);
	if(!vertShader) {
		SDL_Log("Couldn't load vertex shader: %s\n", vertFilename.c_str());
		return false;
	}
	
	GLuint fragShader = loadShaderByType(fragFilename, GL_FRAGMENT_SHADER, extFilename);
	if(!fragShader) {
		SDL_Log("Couldn't load fragent shader: %s\n", fragFilename.c_str());
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
		m_shaderConfig = ShaderConfig();
		createShaderConfigFromUniforms();
		extractUniformMetadata();
		deactivate();
	}
	glBindAttribLocation(m_shaderProgram, 0, "in_Position");
	glBindAttribLocation(m_shaderProgram, 1, "in_TexCoord");

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	return true;
}

bool Shader::load(const std::string& compFilename)
{
	GLuint compShader = loadShaderByType(compFilename, GL_COMPUTE_SHADER);
	if(!compShader) {
		SDL_Log("Couldn't load compute shader: %s\n", compFilename.c_str());
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

GLuint Shader::loadShaderByType(const std::string& filename, GLenum shaderType, const std::string& extFilename)
{
	m_shaderSrc = "";

	GLuint shader;

	std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open: " << filename << std::endl;
		return -1;
    }
    
    std::ostringstream buffer;
    buffer << file.rdbuf();
    m_shaderSrc = std::string(buffer.str());
	file.close();

	// Load extension when existing
	if (!extFilename.empty()) {
		std::ifstream extFile(extFilename);
		if (!extFile.is_open()) {
			std::cerr << "Failed to open: " << extFilename << std::endl;
			return -1;
		}
		
		std::ostringstream extBuffer;
		extBuffer << extFile.rdbuf();
		std::string extShaderSrc(extBuffer.str());
		extFile.close();

		strhlpr::searchAndReplace(m_shaderSrc, "//###EXT_MAIN_DEF###", extShaderSrc);
		strhlpr::searchAndReplace(m_shaderSrc, "//###EXT_MAIN_USE###", "extMain(color, coord);");
		printf("SOURCE:\n %s\n", m_shaderSrc.c_str());
		printf("EXT SOURCE:\n %s\n", extShaderSrc.c_str());
	}
	
	// Create the shader
	shader = glCreateShader(shaderType);
	const char* src = m_shaderSrc.c_str();
	glShaderSource(shader, 1, &src, NULL);
	
	// Compile it
	glCompileShader(shader);
	GLint compileSucceeded = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileSucceeded);
	
	if(!compileSucceeded) {
		SDL_Log("Compilation of shader %s failed:\n", filename.c_str());
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
		m_shaderSrc = "";
	}
	
	return shader;
}

bool Shader::bindUniformLocation(const std::string& locName, GLint unit)
{
	GLint texSamplerUniformLoc = glGetUniformLocation(m_shaderProgram, locName.c_str());
	if (texSamplerUniformLoc < 0) {
		SDL_Log("ERROR: Couldn't get uniform location with this name: %s", locName.c_str());
		return false;
	}
	glUniform1i(texSamplerUniformLoc, unit);
	return true;
}

bool Shader::setValue(const std::string& locName, GLfloat value)
{
	GLint uniformLoc = glGetUniformLocation(m_shaderProgram, locName.c_str());
	if (uniformLoc < 0) {
		SDL_Log("ERROR: Couldn't get uniform location with this name: %s", locName.c_str());
		return false;
	}
	glUniform1f(uniformLoc, value);
	return true;
}

bool Shader::setValue(const std::string& locName, GLint value)
{
	GLint uniformLoc = glGetUniformLocation(m_shaderProgram, locName.c_str());
	if (uniformLoc < 0) {
		SDL_Log("ERROR: Couldn't get uniform location with this name: %s", locName.c_str());
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


