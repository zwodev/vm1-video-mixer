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