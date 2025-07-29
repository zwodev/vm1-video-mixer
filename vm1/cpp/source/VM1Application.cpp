/*
 * Copyright (c) 2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */


#include "VM1Application.h"
#include "VM1DeviceDefinitions.h"
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define KEYBOARD_DEVICE "/dev/input/event6"

VM1Application::VM1Application() :
    m_keyboardControllerSdl(m_registry, m_eventBus),
    m_keyboardControllerLinux(m_registry, m_eventBus),
    m_playbackOperator(m_registry, m_eventBus, m_deviceController),
    m_fileAssignmentWidget(m_registry, m_eventBus),
    m_stbRenderer(DISPLAY_WIDTH, DISPLAY_HEIGHT),
    m_ui(m_stbRenderer, m_eventBus),
    m_menuSystem(m_ui, m_registry, m_eventBus),
    m_deviceController(m_eventBus, m_registry)
{}

VM1Application::~VM1Application()
{
    finalize();
}

bool VM1Application::initialize()
{
    initializeVideo();
    
    //m_audioSystem.initialize();
    m_cameraController.setupDetached();
    m_oledController.setStbRenderer(&m_stbRenderer);
    m_oledController.start();

    // Open VM-1Device
    std::string serialDevice = m_registry.settings().serialDevice;
    if (!m_deviceController.connect(serialDevice))
    {
        printf("Could not connect to VM1-Device on Serial or I2C.\n");
    }

    return true;
}

bool VM1Application::initializeVideo()
{
    m_fd = -1;
    m_isHeadless = false;
    bool success = initSDL(true);
    if (success) {
        if (!initImGui()) {
            SDL_Log("Failed it initialize ImGui!");
            return false;
        }
    }
    else {
        if (initSDL(false)) {
            m_isHeadless = true;
            m_fd = open(KEYBOARD_DEVICE, O_RDONLY | O_NONBLOCK);
            if (m_fd < 0) {
               SDL_Log("Failed it initialize linux keyboard device!");
            }

            SDL_Log("Running in headless mode!");          
        }
        else {
            SDL_Log("Failed to initialize SDL!");
            return false;
        }
    }
    
    if (!m_isHeadless) m_playbackOperator.initialize();
    
    return true;
}

std::vector<SDL_DisplayMode> VM1Application::getBestDisplaysModes() 
{
    std::vector<SDL_DisplayMode> bestDisplayModes;

    int numDisplays;
    SDL_DisplayID* displays = SDL_GetDisplays(&numDisplays);
    SDL_Log("Found %d display(s)", numDisplays);

    for (int i = 0; i < numDisplays; ++i) {
        SDL_Log("Display ID: %d", displays[i]);
        int numModes;
        SDL_DisplayMode** displayModes = SDL_GetFullscreenDisplayModes(displays[i], &numModes);
        if (numModes < 1) {
            SDL_Log("No display modes found!");
            return bestDisplayModes;
        }

        SDL_DisplayMode bestMode = {0};
        bool found = false;
        for (int i = 0; i < numModes; ++i) {
            SDL_DisplayMode mode = *(displayModes[i]);
            //SDL_Log("Mode: %dx%d @%f", mode.w, mode.h, mode.refresh_rate);

            // Only consider modes up to 1920x1080
            if (mode.w <= 1920 && mode.refresh_rate <= 60.0) {
                if (!found ||
                    (mode.w > bestMode.w) ||
                    (mode.w == bestMode.w && mode.h > bestMode.h) ||
                    (mode.w == bestMode.w && mode.h == bestMode.h && mode.refresh_rate > bestMode.refresh_rate)) {
                    bestMode = mode;
                    found = true;
                }
            }
        }

        if (found) bestDisplayModes.push_back(bestMode);
    }
    SDL_free(displays);

    return bestDisplayModes;
}

bool VM1Application::initSDL(bool withVideo)
{
    if (!withVideo) {
        if (!SDL_Init(SDL_INIT_GAMEPAD))
        {
            printf("Error: SDL_Init(): %s\n", SDL_GetError());
            return false;
        }
        return true;
    }


    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD))
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
    m_displayModes = getBestDisplaysModes();
    for (const auto& mode : m_displayModes) {
        SDL_Log("Mode: %dx%d @%f", mode.w, mode.h, mode.refresh_rate);
    }

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
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, 0);

    int xOffset = 0;
    // for (int i = 0; i < m_displayModes.size(); ++i) {
    //     maxX += m_displayModes[i].w;
    // }
    m_windows.resize(m_displayModes.size(), nullptr);
    for (int i = 0; i < m_displayModes.size(); ++i)
    {
        //int index = (m_displayModes.size()-1) - i;
        int index = i;
        SDL_DisplayMode mode = m_displayModes[index];
        //int x = maxX - mode.w;


        // This is the way to associate the second window with the second screen
        // when using the DRM/KMS backend.
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, mode.w);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, mode.h);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, xOffset);
        xOffset += mode.w;
        

        SDL_Window *window = SDL_CreateWindowWithProperties(props);
        if (window == nullptr)
        {
            printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
            finalizeSDL();
            return false;
        }

        m_windows[index] = window;
    }

    for (int i = 0; i < m_windows.size(); ++i) {
        if (!SDL_SetWindowFullscreenMode(m_windows[i], &(m_displayModes[i]))) {
            SDL_Log("Unable to set fullscreen mode!");
        }
    }

    if (m_windows.size() < 1)
    {
        SDL_Log("Unable to create an SDL window!");
        finalizeSDL();
        return false;
    }

    m_glContext = SDL_GL_CreateContext(m_windows[0]);
    if (m_glContext == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        finalizeSDL();
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

    SDL_RaiseWindow(m_windows[0]);

    return true;
}

bool VM1Application::initImGui()
{
    IMGUI_CHECKVERSION();

    m_imguiContext = ImGui::CreateContext();
    ImGui_ImplSDL3_InitForOpenGL(m_windows[0], m_glContext);

    const char *glsl_version = "#version 300 es";
    ImGui_ImplOpenGL3_Init(glsl_version);

    //ImGuiIO &io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    ImGui::StyleColorsDark();

    return true;
}

void VM1Application::finalize()
{
    m_deviceController.disconnect();
    if (!m_isHeadless) finalizeImGui();
    //m_audioSystem.finalize();
    finalizeSDL();
}

void VM1Application::finalizeImGui()
{
    ImGui::SetCurrentContext(m_imguiContext);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext(m_imguiContext);
    m_imguiContext = nullptr;
}

void VM1Application::finalizeSDL()
{
    SDL_GL_DestroyContext(m_glContext);
    for (int i = 0; i < m_windows.size(); ++i)
    {
        if (m_windows[i]) {
            SDL_DestroyWindow(m_windows[i]);
        }
    }
    m_windows.clear();
    m_displayModes.clear();
    SDL_Quit();
}

bool VM1Application::processSDLInput()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        m_keyboardControllerSdl.update(event);
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_ESCAPE) {
                return false;
            }
            else if (event.key.key == SDLK_SPACE) {
                m_playbackOperator.finalize();
                finalize();
                initializeVideo();
                SDL_Log("Reinitialize video!"); 
            }
        }
    }

    return true;
}

bool VM1Application::processLinuxInput()
{
    if (m_fd < 0) return false;

    input_event ev;
    ssize_t n = read(m_fd, &ev, sizeof(ev));
    if (n == (ssize_t)sizeof(ev)) {
        if (ev.type == EV_KEY && ev.value == 1) {
            m_keyboardControllerLinux.update(ev);
            if (ev.code == KEY_ESC) {
                return false;
            }
            else if (ev.code == KEY_SPACE) {
                m_playbackOperator.finalize();
                finalize();
                initializeVideo();
                SDL_Log("Reinitialize video!"); 
            }
        }
    } else if (n == -1 && errno != EAGAIN) {
        return false;
    }

    return true;
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

        // Force to < 60 fps even when in headless mode
        if (deltaTime < 0.016) {
            SDL_Delay(1);
            continue;
        }

        lastTime = currentTime;

        if (m_isHeadless) {
            if (!processLinuxInput()) done = true;
        }
        else {
            if (!processSDLInput()) done = true;
        }

        m_deviceController.requestVM1DeviceBuffer();
        m_registry.update(deltaTime);
        m_playbackOperator.update(deltaTime);
        m_menuSystem.render();
        m_stbRenderer.update();

        if (!m_isHeadless) { 
            renderImGui();

            for (int i = 0; i < m_windows.size(); ++i) {
                renderWindow(i);
            }
        }
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
    ImGui::EndFrame();
}

void VM1Application::renderWindow(int windowIndex)
{   
    // TODO: Move viewport calculation to init method
    // Maybe create struct which has SDL_DisplayMode and SDL_Window?
    SDL_DisplayMode mode = m_displayModes[windowIndex];
    float contentAspect = 16.0f/9.0f;
    float displayAspect = (float)mode.w / (float)mode.h;

    int width = mode.w;
    int height = mode.h;
    int xOffset = 0;
    int yOffset = 0;
    if (displayAspect <= contentAspect) {
        height = int((float)width / contentAspect);
        yOffset = (mode.h - height) / 2;
    }
    else {
        width = int((float)height * contentAspect);
        xOffset = (mode.w - width) / 2;
    }

    SDL_GL_MakeCurrent(m_windows[windowIndex], m_glContext);    
    glViewport(xOffset, yOffset, width, height);
    //glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClearColor(0.0f, 0.0f, 0.0f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (m_registry.settings().showUI) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);;
    }
    else {
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);;
    }

    m_playbackOperator.renderPlane(windowIndex);

    if (!m_isHeadless && windowIndex == 0) {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    SDL_GL_SwapWindow(m_windows[windowIndex]);
}