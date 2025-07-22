/*
 * Copyright (c) 2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */


#pragma once

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengles2.h>

#include "GLHelper.h"
#include "Registry.h"
#include "PlaybackOperator.h"
#include "FileAssignmentWidget.h"
#include "CameraController.h"
#include "OledUiRenderer.h"
#include "OledController.h"
#include "StbRenderer.h"
#include "MenuSystem.h"
#include "UI.h"
#include "KeyboardController.h"
#include "EventBus.h"

class VM1Application {

public:
    VM1Application();
    ~VM1Application();

public:
    bool exec();

private:
    bool initialize();
    bool initSDL(bool withoutVideo);
    bool initImGui();
    void finalize();
    void finalizeImGui();
    void finalizeSDL();
    void renderImGui();
    void renderWindow(int windowIndex);

private:
    SDL_GLContext m_glContext = nullptr;
    std::vector<SDL_Window *> m_windows;
    ImGuiContext* m_imguiContext = nullptr;
    
    Registry m_registry;
    EventBus m_eventBus;
    KeyboardController m_keyboardController;
    PlaybackOperator m_playbackOperator;
    CameraController m_cameraController;
    FileAssignmentWidget m_fileAssignmentWidget;
    StbRenderer m_stbRenderer;
    UI m_ui;
    OledController m_oledController;
    MenuSystem m_menuSystem;
};