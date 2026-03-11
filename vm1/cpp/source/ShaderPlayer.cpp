/*
 * Copyright (c) 2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include "ShaderPlayer.h"
#include "GLHelper.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <iostream>
#include <math.h>
#include <variant>

#include <SDL3/SDL.h>
#include <GLES3/gl31.h>

using namespace std;

const int IMAGE_WIDTH = 1920;
const int IMAGE_HEIGHT = 1080;


ShaderPlayer::ShaderPlayer()
{
    loadShaders();
    createVertexBuffers();
    initializeFramebufferAndTextures();

    //glGenTextures(1, &m_nonZeroCopyTextureId);
}

ShaderPlayer::~ShaderPlayer()
{
    close();  
}

bool ShaderPlayer::openFile(const std::string& fileName, AudioStream* audioStream)
{
    printf("Load Custom Shader: %s\n", fileName.c_str());
    m_isShaderReady = m_shader.load("shaders/pass.vert", fileName.c_str());
    return true;
}

void ShaderPlayer::close()
{
    MediaPlayer::close();

    if (m_isShaderReady) {
        m_shader = Shader();
        m_isShaderReady = false;
    }
}

void ShaderPlayer::finalize()
{
}

void ShaderPlayer::loadShaders()
{
}

void ShaderPlayer::run()
{   
}


void ShaderPlayer::activateShader()
{
    m_shader.activate();
}

void ShaderPlayer::deactivateShader()
{
    m_shader.deactivate();
}

void ShaderPlayer::setCurrentTime(float time)
{
    m_currentTime = time;
    //m_shader.setValue("iTime", time);
}

void ShaderPlayer::setAnalogValue(float value) 
{
    m_analogValue = value;
}

void ShaderPlayer::render()
{
    glViewport(0, 0, 1920, 1080);
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    activateShader();    

    glBindVertexArray(m_vao);

    //m_shader.bindUniformLocation("inputTexture", 0);
    //m_shader.setValue("iTime", m_currentTime);
    //m_shader.setValue("iAnalog0", m_analogValue);

    for (const auto& param : m_shaderConfig.params) {
        if (std::holds_alternative<IntParameter>(param)) {
            auto& intParam = std::get<IntParameter>(param);  
            m_shader.setValue(intParam.name.c_str(), intParam.value);
        } else if (std::holds_alternative<FloatParameter>(param)) {
            auto& floatParam = std::get<FloatParameter>(param); 
            m_shader.setValue(floatParam.name.c_str(), floatParam.value);
        }
    }
    
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    deactivateShader();

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShaderPlayer::update()
{
    if (m_isShaderReady) render();
}

const ShaderConfig& ShaderPlayer::shaderConfig()
{
    return m_shader.shaderConfig();
}

void ShaderPlayer::setShaderUniforms(const ShaderConfig& shaderConfig)
{
    m_shaderConfig = shaderConfig;
}