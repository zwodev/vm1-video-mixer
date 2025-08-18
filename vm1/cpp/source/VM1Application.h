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
#include "OledController.h"
#include "StbRenderer.h"
#include "MenuSystem.h"
#include "UI.h"
#include "KeyboardControllerSDL.h"
#include "KeyboardControllerLinux.h"
#include "EventBus.h"
#include "DeviceController.h"
#include "AudioSystem.h"

struct DisplayConf {
    SDL_DisplayMode defaultMode = {};
    SDL_DisplayMode bestMode = {};
};

class VM1Application {

public:
    VM1Application();
    ~VM1Application();

public:
    bool exec();

private:
    void subscribeToEvents();
    bool initialize();
    bool initializeVideo();
    std::vector<DisplayConf> getBestDisplaysConfigs();
    bool initSDL(bool withoutVideo);
    bool initImGui();
    void finalize();
    void finalizeImGui();
    void finalizeSDL();

    bool processSDLInput();
    bool processLinuxInput();
    void renderImGui();
    void renderWindow(int windowIndex);

private:
    int m_fd = -1;
    bool m_isHeadless = true;
    SDL_GLContext m_glContext = nullptr;
    std::vector<SDL_Window *> m_windows;
    std::vector<DisplayConf> m_displayConfigs;
    ImGuiContext* m_imguiContext = nullptr;
    
    Registry m_registry;
    EventBus m_eventBus;
    KeyboardControllerSDL m_keyboardControllerSdl;
    KeyboardControllerLinux m_keyboardControllerLinux;
    PlaybackOperator m_playbackOperator;
    CameraController m_cameraController;
    FileAssignmentWidget m_fileAssignmentWidget;
    StbRenderer m_stbRenderer;
    UI m_ui;
    OledController m_oledController;
    MenuSystem m_menuSystem;
    DeviceController m_deviceController;
};