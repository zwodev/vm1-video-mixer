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
    m_isShaderReady = m_customShader.load("shaders/pass.vert", fileName.c_str());
    return true;
}

void ShaderPlayer::close()
{
    MediaPlayer::close();

    if (m_isShaderReady) {
        m_customShader = Shader();
        m_isShaderReady = false;
    }
}

void ShaderPlayer::finalize()
{
}

void ShaderPlayer::loadShaders()
{
    //printf("Load Shaders!\n");
    //m_customShader.load("shaders/pass.vert", "shaders/custom.frag");
}

void ShaderPlayer::run()
{   
}


void ShaderPlayer::activateShader()
{
    m_customShader.activate();
}

void ShaderPlayer::deactivateShader()
{
    m_customShader.deactivate();
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

    // glActiveTexture(GL_TEXTURE0);
    // if (m_captureType == CaptureType::CT_CSI || m_captureType == CaptureType::CT_WEBCAM) {
    //     glBindTexture(GL_TEXTURE_2D, m_yuvTextures[0]);
    //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //     GLHelper::glEGLImageTargetTexture2DOESFunc(GL_TEXTURE_2D, m_yuvImages[0]);
    // }
    // else {
    //     glBindTexture(GL_TEXTURE_2D, m_nonZeroCopyTextureId);
    //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // }
    //m_shader.bindUniformLocation("inputTexture", 0);
    m_customShader.setValue("iTime", m_currentTime);
    m_customShader.setValue("iAnalog0", m_analogValue);
    

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
