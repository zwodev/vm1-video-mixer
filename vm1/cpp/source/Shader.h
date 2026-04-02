/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include <GLES3/gl3.h>

#include <ShaderConfig.h>

class Shader {
public:
    Shader();
    ~Shader();

public:
    bool load(const std::string& vertFilename, const std::string& fragFilename, const std::string& extFilename = "");
    bool load(const std::string& compFilename);
    bool bindUniformLocation(const std::string& locName, GLint unit);
    bool setValue(const std::string& locName, GLfloat value);
    bool setValue(const std::string& locName, GLint value);
    void activate();
    void deactivate();
    const ShaderConfig& shaderConfig();
    
private:
    GLuint loadShaderByType(const std::string& filename, GLenum shaderType, const std::string& extFilename = "");
    bool link();
    void createShaderConfigFromUniforms();
    void parseUnifromJson(const std::string& uniformName, const std::string& uniformType, const std::string& uniformJson);
    void extractUniformMetadata();
    void destroyShaderProg(GLuint shaderProg);
    

private:
    GLuint m_shaderProgram = 0;
    ShaderConfig m_shaderConfig;
    std::string m_shaderSrc;
};