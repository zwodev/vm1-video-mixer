/*
 * Copyright (c) 2023-2024 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include <GLES3/gl3.h>

class Shader {
public:
    Shader();
    ~Shader();

public:
    bool load(const char *vertFilename, const char *fragFilename);
    bool bindUniformLocation(const char* locName, GLint unit);
    void activate();
    void deactivate();
    
private:
    GLuint loadShaderByType(const char *filename, GLenum shaderType);
    void destroyShaderProg(GLuint shaderProg);

private:
    GLuint m_shaderProgram = 0;
};