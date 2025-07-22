/*
 * Copyright (c) 2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */


#include "VM1Application.h"

VM1Application::VM1Application() :
    m_keyboardController(m_registry, m_eventBus),
    m_playbackOperator(m_registry),
    m_fileAssignmentWidget(m_playbackOperator, m_registry),
    m_stbRenderer(128, 128),
    m_ui(m_stbRenderer, m_eventBus),
    m_menuSystem(m_ui, m_registry, m_eventBus)
{

}

VM1Application::~VM1Application()
{

}

bool VM1Application::initialize()
{
    if (!initSDL(false)) {
        SDL_Log("Failed to initialize SDL!");
        return false;
    }

    if (!initImGui()) {
        SDL_Log("Failed it initialize ImGui!");
        return false;
    }
    
    m_playbackOperator.initialize();
    m_cameraController.setupDetached();
    m_oledController.setStbRenderer(&m_stbRenderer);
    m_oledController.start();

    return true;
}

bool VM1Application::initSDL(bool withoutVideo)
{
    if (withoutVideo) {
        if (!SDL_Init(SDL_INIT_GAMEPAD))
        {
            printf("Error: SDL_Init(): %s\n", SDL_GetError());
            return false;
        }
        return true;
    }


    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return false;
    }

    // Setup OpenGL context for OpenGL ES 3.1
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
    SDL_free(displays);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_PropertiesID props = SDL_CreateProperties();
    if (props == 0)
    {
        SDL_Log("Unable to create properties: %s", SDL_GetError());
        return false;
    }

    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Window");
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_WAYLAND_CREATE_EGL_WINDOW_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 1920);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 1200);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, 0);

    for (int i = 0; i < num_displays; ++i)
    {
        // This is the way to associate the second window with the second screen
        // when using the DRM/KMS backend.
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, 1920 * i);

        SDL_Window *window = SDL_CreateWindowWithProperties(props);
        if (window == nullptr)
        {
            printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
            return false;
        }
        m_windows.push_back(window);
    }

    if (m_windows.size() < 1)
    {
        SDL_Log("Unable to create an SDL window!");
        return false;
    }

    m_glContext = SDL_GL_CreateContext(m_windows[0]);
    if (m_glContext == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return false;
    }

    bool success = GLHelper::init();
    printf("Init GL Helper: %d\n", success);

    // Enable vsync and activate all windows
    SDL_GL_SetSwapInterval(1);
    for (int i = 0; i < m_windows.size(); ++i)
    {
        SDL_ShowWindow(m_windows[i]);
    }

    return true;
}

bool VM1Application::initImGui()
{
    IMGUI_CHECKVERSION();

    m_imguiContext = ImGui::CreateContext();
    ImGui_ImplSDL3_InitForOpenGL(m_windows[0], m_glContext);

    const char *glsl_version = "#version 300 es";
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImGui::StyleColorsDark();
    //SDL_RaiseWindow(windows[0]);

    return true;
}

void VM1Application::finalize()
{
    finalizeImGui();
    finalizeSDL();
}

void VM1Application::finalizeImGui()
{
    ImGui::SetCurrentContext(m_imguiContext);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext(m_imguiContext);
}

void VM1Application::finalizeSDL()
{
    SDL_GL_DestroyContext(m_glContext);
    for (int i = 0; i < m_windows.size(); ++i)
    {
        SDL_DestroyWindow(m_windows[i]);
    }
    SDL_Quit();
}

bool VM1Application::exec()
{
    if (!initialize()) return false;

    Uint64 lastTime = SDL_GetTicks();
    bool done = false;
    
    while (!done)
    {
        Uint64 currentTime = SDL_GetTicks();
        double deltaTime = (currentTime - lastTime) / 1000.0;
        lastTime = currentTime;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            m_keyboardController.update(event);
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                if (event.key.key == SDLK_ESCAPE) {
                    done = true;
                }
            }
            if (SDL_GetWindowFlags(m_windows[0]) & SDL_WINDOW_MINIMIZED)
            {
                SDL_Delay(10);
                continue;
            }
        }

        m_registry.update(deltaTime);
        m_playbackOperator.update(deltaTime);
        m_menuSystem.render();
        m_stbRenderer.update();

        renderImGui();

        for (int i = 0; i < m_windows.size(); ++i) {
            renderWindow(i);
        }

        // End the frame 
        // TODO: Can this be moved to "renderImGui"?
        ImGui::EndFrame();
    }

    finalize();

    return true;
}

void VM1Application::renderImGui()
{
    // Start the Dear ImGui frame
    ImGui::SetCurrentContext(m_imguiContext);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();


    // Desktop UI
    // TODO: Put in own class.
    if (m_registry.settings().showUI)
    {     
        {
            ImGui::Begin("Development");
            if (ImGui::Button("Save Registry"))
            {
                m_registry.save();
            }
            if (ImGui::Button("Setup HDMI2CSI"))
            {
               m_cameraController.setupDetached();
            }
            ImGuiIO &io = ImGui::GetIO();
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
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

    m_fileAssignmentWidget.render();
}

void VM1Application::renderWindow(int windowIndex)
{        
        SDL_GL_MakeCurrent(m_windows[windowIndex], m_glContext);
        ImGui::Render();
        glViewport(0, 60, 1920, 1080);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);

        m_playbackOperator.renderPlane(windowIndex);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(m_windows[windowIndex]);
}