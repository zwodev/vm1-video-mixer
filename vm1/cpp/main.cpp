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

#include "source/Registry.h"
#include "source/PlaneRenderer.h"
#include "source/VideoPlayer.h"
#include "source/VideoPlane.h"
#include "source/CameraRenderer.h"
#include "source/FileAssignmentWidget.h"
#include "source/MenuSystem.h"
#include "source/KeyForwarder.h"
#include "source/CameraController.h"
#include "source/OledUiRenderer.h"
#include "source/OledController.h"

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
    int num_displays;
    SDL_DisplayID *displays = SDL_GetDisplays(&num_displays);
    SDL_Log("Found %d display(s)", num_displays);
    for (int i = 0; i < num_displays; ++i)
    {
        SDL_Log("Display ID for %d: %d", i, displays[i]);
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
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 1080);
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
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup Platform/Renderer backends for main context
    ImGui_ImplSDL3_InitForOpenGL(windows[0], gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup FBO context for ImGui (off-screen)
    ImGuiContext *fboContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(fboContext);
    ImGuiIO &fbo_io = ImGui::GetIO();
    (void)fbo_io;
    fbo_io.DisplaySize = ImVec2(FBO_WIDTH, FBO_HEIGHT);
    //fbo_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Platform/Renderer backends for main context
    ImGui_ImplSDL3_InitForOpenGL(windows[0], gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // TODO: This was not implemented in SDL3 yet. I did it. Another way or PR?
    SDL_RaiseWindow(windows[0]);

    // Video players
    VideoPlane videoPlane0;
    VideoPlane videoPlane1;

    // Camera
    CameraController cameraController;
    // cameraController.setup();

    CameraRenderer cameraRenderer0;
    // cameraRenderer0.start();
    
    Registry registry;

    // Oled
    OledUiRenderer oledUiRenderer(registry, FBO_WIDTH, FBO_HEIGHT);
    oledUiRenderer.initialize();
    
    
    // Keyforwarding
    KeyForwarder keyForwarder;

#ifdef USE_OLED
    OledController oledController;
    oledController.initializeOled();
    oledController.initializeImageBuffer();
    // oledController.drawTestBMP();
    // oledController.initializeExternalFboTexture(&oledUiRenderer.m_fboTexture);
#endif

    // File Assignment Widget
    FileAssignmentWidget fileAssignmentWidget(registry, "../videos/", &videoPlane0, &videoPlane1);

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
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
            {
                done = true;
            }
            else if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(windows[0]))
            {
                done = true;
            }
        }
        if (SDL_GetWindowFlags(windows[0]) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        keyForwarder.ForwardArrowKeys(mainContext, fboContext);

        // START: Render to FBO (OLED) before main gui
        ImGui::SetCurrentContext(fboContext);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        oledUiRenderer.update();
        //  END: Render to FBO (OLED) before main gui

        // Start the Dear ImGui frame
        ImGui::SetCurrentContext(mainContext);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Test Window
        //menuTest.render();

        // Development Window
        // TODO: Put in own class.
        {
            ImGui::Begin("Development");
            // ImGui::Image((ImTextureID)(intptr_t)oledUiRenderer.texture(), ImVec2(128, 128));
            static float fadeTimeInSecs = 2.0f;
            if (ImGui::SliderFloat("Fade Time", &fadeTimeInSecs, 0.0f, 5.0f))
            {
                videoPlane0.setFadeTime(fadeTimeInSecs);
                videoPlane1.setFadeTime(fadeTimeInSecs);
            }
            ImGui::Checkbox("Enable Video", &isVideoEnabled);
            if (ImGui::Button("Setup HDMI2CSI"))
            {
                cameraController.setup();
            }
            if (ImGui::Button("Start HDMI2CSI"))
            {
                cameraRenderer0.start();
            }
            ImGui::Checkbox("Show HDMI2CSI", &isCameraEnabled);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // OLED debug window
        {
            // ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(FBO_WIDTH, FBO_HEIGHT));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration;
            ImGui::Begin("OLED Debug Window", nullptr, window_flags);
            // ImGui::Begin("OLED Debug Window");
            ImGui::Image((void *)(intptr_t)oledUiRenderer.texture(), ImVec2(FBO_WIDTH, FBO_HEIGHT), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
            ImGui::End();
            ImGui::PopStyleVar();
        }

        // File Assignment Widget
        fileAssignmentWidget.render();

        // Rendering window 0
        SDL_GL_MakeCurrent(windows[0], gl_context);
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render video content
        if (isVideoEnabled)
            videoPlane0.update(deltaTime);

        // Render camera content
        if (isCameraEnabled)
            cameraRenderer0.update();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(windows[0]);

        // Rendering window 1
        if (windows.size() > 1)
        {
            SDL_GL_MakeCurrent(windows[1], gl_context);
            glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
            glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);

            // Render video content
            if (isVideoEnabled)
                videoPlane1.update(deltaTime);

            SDL_GL_SwapWindow(windows[1]);
        }

        // Render OLED
    #ifdef USE_OLED
        oledUiRenderer.renderToRGB565(oledController.oledImage);
        oledController.render();
    #endif

        // End the frame
        ImGui::EndFrame();
    }

    // Cleanup
    ImGui::SetCurrentContext(fboContext);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();

    ImGui::SetCurrentContext(mainContext);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();

    ImGui::DestroyContext(fboContext);
    ImGui::DestroyContext(mainContext);

    SDL_GL_DestroyContext(gl_context);
    for (int i = 0; i < windows.size(); ++i)
    {
        SDL_DestroyWindow(windows[i]);
    }
    SDL_Quit();

    return 0;
}

// EXAMPLE: ImGUI + SDL3 + OpenGL ES
// (with 2 ImGui Contexts, one rendering to screen and one to FBO)

/*
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengles2.h>
#include <SDL3/SDL_egl.h>
#include <stdio.h>
//#include <GL/gl3w.h>

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int FBO_WIDTH = 600;
const int FBO_HEIGHT = 600;

GLuint createFramebuffer(int width, int height, GLuint& textureColorbuffer) {
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return framebuffer;
}

int main(int, char**)
{
    printf("Hello!");
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("ImGui SDL3+OpenGL3 example", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);

    const char* glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    IMGUI_CHECKVERSION();
    ImGuiContext* mainContext = ImGui::CreateContext();
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGuiContext* secondContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(secondContext);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(FBO_WIDTH, FBO_HEIGHT);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui::SetCurrentContext(mainContext);

    GLuint fboTexture;
    GLuint fbo = createFramebuffer(FBO_WIDTH, FBO_HEIGHT, fboTexture);

    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
        }

        ImGui::SetCurrentContext(secondContext);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();

        // Main context (render to FBO)
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, FBO_WIDTH, FBO_HEIGHT);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(FBO_WIDTH, FBO_HEIGHT));
        ImGui::Begin("FBO Window", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
        ImGui::Text("This is rendered to FBO");
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        ImGui::SetCurrentContext(mainContext);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();

        // Second context (render to screen)
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);


        ImGui::NewFrame();
        ImGui::Begin("Screen Window");
        ImGui::Text("This is rendered to screen");
        ImGui::Image((void*)(intptr_t)fboTexture, ImVec2(FBO_WIDTH, FBO_HEIGHT));
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    ImGui::SetCurrentContext(secondContext);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();

    ImGui::SetCurrentContext(mainContext);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();

    ImGui::DestroyContext(secondContext);
    ImGui::DestroyContext(mainContext);

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
*/