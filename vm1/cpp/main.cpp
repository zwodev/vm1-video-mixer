/*
 * Copyright (c) 2023-2025 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 *
 * Parts of this file have been taken from:
 * https://github.com/libsdl-org/SDL/blob/main/test/testffmpeg.c
 *
 */

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

#include <stdio.h>
#include <SDL3/SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

#include "source/GLHelper.h"
#include "source/Registry.h"
#include "source/PlaybackOperator.h"
#include "source/PlaneRenderer.h"
#include "source/VideoPlayer.h"
#include "source/CameraPlayer.h"
#include "source/FileAssignmentWidget.h"
#include "source/KeyForwarder.h"
#include "source/CameraController.h"
#include "source/OledUiRenderer.h"
#include "source/OledController.h"
#include "source/StbRenderer.h"
#include "source/MenuSystem.h"
#include "source/UIHelper.h"
#include "source/KeyboardController.h"
#include "source/EventBus.h"
#include "source/EventHandler.h"

#define USE_OLED

const int FBO_WIDTH = 128;
const int FBO_HEIGHT = 128;

// Main code
int main(int, char **)
{
    // Setup SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    // Setup OpenGL context for OpenGL ES 3.1
    const char *glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    // Check number of attached displays
    int x_offsets[2] = { 0, 0 };
    int y_offsets[2] = { 0, 0 };
    int num_displays;
    SDL_DisplayID *displays = SDL_GetDisplays(&num_displays);
    SDL_Log("Found %d display(s)", num_displays);
    for (int i = 0; i < num_displays; ++i)
    {
        SDL_Log("Display ID for %d: %d", i, displays[i]);
        // SDL_DisplayMode mode;
        // if (SDL_GetCurrentDisplayMode(displays[i], &mode) == 0) {
        //     SDL_Log("Display %d native resolution: %dx%d @ %dHz\n",
        //            i, mode.w, mode.h, mode.refresh_rate);
        // }
    }
    SDL_free(displays);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_PropertiesID props = SDL_CreateProperties();
    if (props == 0)
    {
        SDL_Log("Unable to create properties: %s", SDL_GetError());
        return 0;
    }

    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Window");
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_WAYLAND_CREATE_EGL_WINDOW_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 1920);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 1200);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, 0);

    std::vector<SDL_Window *> windows;
    for (int i = 0; i < num_displays; ++i)
    {
        // This is the way to associate the second window with the second screen
        // when using the DRM/KMS backend.
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, 1920 * i);

        SDL_Window *window = SDL_CreateWindowWithProperties(props);
        if (window == nullptr)
        {
            printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
            return -1;
        }
        windows.push_back(window);
    }

    if (windows.size() < 1)
    {
        SDL_Log("Unable to create an SDL window!");
        return 0;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(windows[0]);
    if (gl_context == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return -1;
    }

    // Init GL Helper
    bool success = GLHelper::init();
    printf("Init GL Helper: %d\n", success);

    // Enable vsync and activate all windows
    SDL_GL_SetSwapInterval(1);
    for (int i = 0; i < windows.size(); ++i)
    {
        SDL_ShowWindow(windows[i]);
    }

    IMGUI_CHECKVERSION();

    // Setup main context for ImGui (screen)
    ImGuiContext *mainContext = ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    // Setup Platform/Renderer backends for main context
    ImGui_ImplSDL3_InitForOpenGL(windows[0], gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup FBO context for ImGui (off-screen)
    // ImGuiContext *fboContext = ImGui::CreateContext();
    // ImGui::SetCurrentContext(fboContext);
    // ImGuiIO &fbo_io = ImGui::GetIO();
    // (void)fbo_io;
    // fbo_io.DisplaySize = ImVec2(FBO_WIDTH, FBO_HEIGHT);

    // // Setup Platform/Renderer backends for main context
    // ImGui_ImplSDL3_InitForOpenGL(windows[0], gl_context);
    // ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // TODO: This was not implemented in SDL3 yet. I did it. Another way or PR?
    SDL_RaiseWindow(windows[0]);    

    // Create registry
    Registry registry;

    // Playback operator
    PlaybackOperator playbackOperator(registry);

    // Camera
    CameraController cameraController;
    cameraController.setupDetached();

    // File Assignment Widget
    FileAssignmentWidget fileAssignmentWidget(playbackOperator, registry);

    // Keyforwarding
    //KeyForwarder keyForwarder;

    // Oled
    // OledUiRenderer oledUiRenderer(registry, FBO_WIDTH, FBO_HEIGHT);
    // oledUiRenderer.initialize();
    StbRenderer stbRenderer(FBO_WIDTH, FBO_HEIGHT);
    UI::SetRenderer(&stbRenderer);

    // Keyboard Controller
    KeyboardController keyboardController;

    MenuSystem menuSystem(registry);
    

#ifdef USE_OLED
    OledController oledController;
    oledController.setStbRenderer(&stbRenderer);
    oledController.start();
#endif

    // Prepared delta time
    Uint64 lastTime = SDL_GetTicks();
    bool isVideoEnabled = true;
    bool isCameraEnabled = false;

    std::string selectedFile;

    // Main loop
    bool done = false;
    while (!done)
    {
        // Calculate delta time
        Uint64 currentTime = SDL_GetTicks();
        double deltaTime = (currentTime - lastTime) / 1000.0;
        lastTime = currentTime;

        // Poll and handle events (inputs, window resize, etc.)
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            keyboardController.update(event);

            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
            {
                done = true;
            }
            else if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(windows[0]))
            {
                done = true;
            }
            else if (event.type == SDL_EVENT_DISPLAY_ADDED)
            {
                SDL_Log("Display added!\n");
            }
            else if (event.type == SDL_EVENT_DISPLAY_REMOVED)
            {
                SDL_Log("Display removed!\n");
            }
        }
        if (SDL_GetWindowFlags(windows[0]) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        registry.update(deltaTime);
        playbackOperator.update(deltaTime);
        //keyForwarder.forwardArrowKeys(mainContext, fboContext);

        // START: Render to FBO (OLED) before main gui
        // ImGui::SetCurrentContext(fboContext);
        // ImGui_ImplOpenGL3_NewFrame();
        // ImGui_ImplSDL3_NewFrame();
        // ImGui::NewFrame();
        menuSystem.render();
        stbRenderer.update();
        //  END: Render to FBO (OLED) before main gui

        // Start the Dear ImGui frame
        ImGui::SetCurrentContext(mainContext);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();


        // Desktop UI
        // TODO: Put in own class.
        if (registry.settings().showUI)
        {     
            {
                ImGui::Begin("Development");
                // ImGui::Image((ImTextureID)(intptr_t)oledUiRenderer.texture(), ImVec2(128, 128));
                static float fadeTimeInSecs = 2.0f;
                if (ImGui::SliderFloat("Fade Time", &fadeTimeInSecs, 0.0f, 5.0f))
                {
                    //videoPlane0.setFadeTime(fadeTimeInSecs);
                    //videoPlane1.setFadeTime(fadeTimeInSecs);
                }
                if (ImGui::Button("Save Registry"))
                {
                    registry.save();
                }
                ImGui::Checkbox("Enable Video", &isVideoEnabled);
                if (ImGui::Button("Setup HDMI2CSI"))
                {
                    cameraController.setupDetached();
                }
                // if (ImGui::Button("Start HDMI2CSI"))
                // {
                //     cameraRenderer0.start();
                // }
                ImGui::Checkbox("Show HDMI2CSI", &isCameraEnabled);
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
                ImGui::End();
                //printf("FPS: %f\n", io.Framerate);
            }

            // OLED debug window
            // {
            //     // ImGui::SetNextWindowPos(ImVec2(0, 0));
            //     ImGui::SetNextWindowSize(ImVec2(FBO_WIDTH, FBO_HEIGHT));
            //     ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            //     ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration;
            //     ImGui::Begin("OLED Debug Window", nullptr, window_flags);
            //     // ImGui::Begin("OLED Debug Window");
            //     ImGui::Image((void *)(intptr_t)stbRenderer.texture(), ImVec2(FBO_WIDTH, FBO_HEIGHT), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
            //     ImGui::End();
            //     ImGui::PopStyleVar();
            // }
        }

        // File Assignment Widget
        fileAssignmentWidget.render();
        
        // Rendering window 0
        SDL_GL_MakeCurrent(windows[0], gl_context);
        ImGui::Render();
        //glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glViewport(0, 60, 1920, 1080);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render video content
        if (isVideoEnabled) playbackOperator.renderPlane(0);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(windows[0]);

        // Rendering window 1
        if (windows.size() > 1)
        {
            SDL_GL_MakeCurrent(windows[1], gl_context);
            glViewport(0, 60, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
            glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);

            // Render video content
            if (isVideoEnabled) playbackOperator.renderPlane(1);

            SDL_GL_SwapWindow(windows[1]);
        }

        // End the frame
        ImGui::EndFrame();
    }

    // Cleanup
    // ImGui::SetCurrentContext(fboContext);
    // ImGui_ImplOpenGL3_Shutdown();
    // ImGui_ImplSDL3_Shutdown();

    ImGui::SetCurrentContext(mainContext);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();

    ImGui::DestroyContext(mainContext);
    // ImGui::DestroyContext(fboContext);

    SDL_GL_DestroyContext(gl_context);
    for (int i = 0; i < windows.size(); ++i)
    {
        SDL_DestroyWindow(windows[i]);
    }
    SDL_Quit();

    return 0;
}
