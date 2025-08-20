/*
 * Copyright (c) 2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */


#include "VM1Application.h"
#include "VM1DeviceDefinitions.h"

#include <kms++/card.h>
#include <kms++/connector.h>
#include <kms++/videomode.h>

#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

VM1Application::VM1Application() :
    m_keyboardControllerSdl(m_registry, m_eventBus),
    m_keyboardControllerLinux(m_registry, m_eventBus),
    m_playbackOperator(m_registry, m_eventBus, m_deviceController),
    m_fileAssignmentWidget(m_registry, m_eventBus),
    m_stbRenderer(DISPLAY_WIDTH, DISPLAY_HEIGHT),
    m_ui(m_stbRenderer, m_eventBus),
    m_menuSystem(m_ui, m_registry, m_eventBus),
    m_deviceController(m_eventBus, m_registry),
    m_cameraController(m_eventBus)
{
    subscribeToEvents();
}

VM1Application::~VM1Application()
{
    //finalize();
}

void VM1Application::subscribeToEvents()
{
    m_eventBus.subscribe<SystemEvent>([this](const SystemEvent& event) {
        if (event.type == SystemEvent::Type::Restart) {
            finalize();
            initialize();
        } 
        else if (event.type == SystemEvent::Type::Exit) {
            m_done = true;
        }
    });

    m_eventBus.subscribe<HdmiCaptureInitEvent>([this](const HdmiCaptureInitEvent& event) {
        m_registry.settings().hdmiInputs[0] = event.configString;
        m_registry.settings().isHdmiInputReady = true;
    });
}

bool VM1Application::initialize()
{
    initializeVideo();
    
    if (m_isHeadless) m_keyboardHotplug.start();

    m_cameraController.setupDetached();

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
            SDL_Log("Failed initialize ImGui!");
            return false;
        }
    }
    else {
        if (initSDL(false)) {
            m_isHeadless = true;
            SDL_Log("Running in headless mode!");          
        }
        else {
            SDL_Log("Failed to initialize SDL!");
            return false;
        }
    }
    
    if (!m_isHeadless) {
        m_playbackOperator.initialize();
    } 
    
    m_registry.settings().isHdmiOutputReady = true;
    m_registry.settings().hdmiOutputs = std::vector<std::string>(2, std::string());
    for (const auto& displayConf : m_displayConfigs) {
        SDL_DisplayMode mode = displayConf.bestMode;
        std::string configString = std::to_string(mode.w) + "x" + std::to_string(mode.h) + "/" + std::to_string(int(std::round(mode.refresh_rate))) + "Hz";
        if (displayConf.name == "HDMI-A-1")
            m_registry.settings().hdmiOutputs[0] = configString;
        else if (displayConf.name == "HDMI-A-2")
            m_registry.settings().hdmiOutputs[1] = configString;
    }

    return true;
}

bool getDefaultMode(int id, SDL_DisplayMode& defaultDisplayMode)
{
    int fd = open("/dev/dri/card1", O_RDWR | O_CLOEXEC);
    if (fd < 0) { std::cerr << "Failed open.\n"; return false; }

    kms::Card card(fd, true);

    auto connectors = card.get_connectors();
    // Assume 'connectors' is some container or pointer array; iterate appropriately

    std::string connectorName = "HDMI-A-" + std::to_string(id+1);
    kms::Connector* connector = nullptr;
    for (auto c : connectors) {    
        if (c->fullname() == connectorName && c->connected()) {
            std::cout << c->fullname() << std::endl;
            connector = c;
            break;
        }
    }

    if (!connector) {
        std::cerr << connectorName << " connector not found\n";
        close(fd);
        return false;
    }

    kms::Videomode defaultMode = connector->get_default_mode();
    defaultDisplayMode.w = defaultMode.hdisplay;
    defaultDisplayMode.h = defaultMode.vdisplay;
    
    std::cerr << defaultMode.hdisplay << "x" << defaultMode.vdisplay << "\n";

    close(fd);

    return true;
}

std::vector<DisplayConf> VM1Application::getBestDisplaysConfigs() 
{
    std::vector<DisplayConf> bestDisplayConfigs;

    int numDisplays;
    SDL_DisplayID* displays = SDL_GetDisplays(&numDisplays);
    SDL_Log("Found %d display(s)", numDisplays);

    for (int i = 0; i < numDisplays; ++i) {
        SDL_Log("Display ID: %d", displays[i]);
        const char *name = SDL_GetDisplayName(displays[i]);
        std::string displayName;
        if (name) {
            displayName = std::string(name);
            std::cout << displayName << std::endl;
        }

        SDL_DisplayMode* defaultDisplayMode = SDL_GetCurrentDisplayMode(displays[i]);
        if (!defaultDisplayMode) {
            SDL_Log("Could not get default display mode!");
            return bestDisplayConfigs;
        }

        SDL_DisplayMode defaultMode = *(defaultDisplayMode);
        getDefaultMode(displays[i], defaultMode);

        SDL_Log("Default Mode: %dx%d @%f", defaultMode.w, defaultMode.h, defaultMode.refresh_rate);

        int numModes;
        SDL_DisplayMode** displayModes = SDL_GetFullscreenDisplayModes(displays[i], &numModes);
        if (numModes < 1) {
            SDL_Log("No display modes found!");
            return bestDisplayConfigs;
        }

        SDL_DisplayMode bestMode = {0};
        bool found = false;
        for (int i = 0; i < numModes; ++i) {
            SDL_DisplayMode mode = *(displayModes[i]);
            SDL_Log("Mode: %dx%d @%f", mode.w, mode.h, mode.refresh_rate);

            // Only consider modes up to 1920x1080
            if (mode.w <= 1920 && mode.refresh_rate <= 61.0) {
                if (!found ||
                    (mode.w > bestMode.w) ||
                    (mode.w == bestMode.w && mode.h > bestMode.h) ||
                    (mode.w == bestMode.w && mode.h == bestMode.h && mode.refresh_rate > bestMode.refresh_rate)) {
                    bestMode = mode;
                    found = true;
                }
            }
        }

        if (found)  {
            DisplayConf displayConfig;
            displayConfig.name = displayName;
            displayConfig.bestMode = bestMode;
            displayConfig.defaultMode = defaultMode;
            bestDisplayConfigs.push_back(displayConfig);
        }
        SDL_free(displayModes);
    }
    SDL_free(displays);

    return bestDisplayConfigs;
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
    m_displayConfigs = getBestDisplaysConfigs();
    for (const auto& config : m_displayConfigs) {
        SDL_Log("Mode: %dx%d @%f", config.bestMode.w, config.bestMode.h, config.bestMode.refresh_rate);
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
    m_windows.resize(m_displayConfigs.size(), nullptr);
    for (int i = 0; i < m_displayConfigs.size(); ++i)
    {
        int index = i;
        SDL_DisplayMode mode = m_displayConfigs[index].bestMode;

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
        if (!SDL_SetWindowFullscreenMode(m_windows[i], &(m_displayConfigs[i].bestMode))) {
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

    ImGui::StyleColorsDark();

    return true;
}

void VM1Application::finalize()
{
    m_playbackOperator.finalize();
    m_deviceController.disconnect();
    if (!m_isHeadless) finalizeImGui();
    if (m_isHeadless) m_keyboardHotplug.stop();
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
    for (int i = 0; i < m_windows.size(); ++i)
    {
        if (m_windows[i]) {
            SDL_DestroyWindow(m_windows[i]);
        }
    }
    SDL_GL_DestroyContext(m_glContext);
    m_windows.clear();
    m_displayConfigs.clear();

    SDL_Quit();
}

bool VM1Application::processSDLInput()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        m_keyboardControllerSdl.update(event);
    }

    return true;
}

bool VM1Application::processLinuxInput()
{
    std::vector<input_event> events = m_keyboardHotplug.getAllEvents();
    for (auto& ev : events) {
        m_keyboardControllerLinux.update(ev);
    }

    return true;
}

bool VM1Application::exec()
{
    if (!initialize()) return false;

    m_oledController.setStbRenderer(&m_stbRenderer);
    m_oledController.start();

    Uint64 lastTime = SDL_GetTicks();
    
    while (!m_done)
    {
        Uint64 currentTime = SDL_GetTicks();
        double deltaTime = (currentTime - lastTime) / 1000.0;

        // Force to < 60 fps even when in headless mode
        if (deltaTime < 0.016) {
            SDL_Delay(1);
            continue;
        }

        lastTime = currentTime;

        m_eventBus.processEvents();

        if (m_isHeadless) processLinuxInput();
        else processSDLInput();

        if (!m_done) {
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
    }

    finalize();

    SDL_Delay(10);

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
               //m_cameraController.setupDetached();
            }
            ImGuiIO &io = ImGui::GetIO();
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }
    }

    m_fileAssignmentWidget.render();
    ImGui::EndFrame();
}

void VM1Application::renderWindow(int windowIndex)
{   
    // TODO: Move viewport calculation to init method
    // Maybe create struct which has SDL_DisplayMode and SDL_Window?
    SDL_DisplayMode bestMode = m_displayConfigs[windowIndex].bestMode;
    SDL_DisplayMode defaultMode = m_displayConfigs[windowIndex].defaultMode;    
    float contentAspect = 16.0f/9.0f;
    float displayAspect = (float)bestMode.w / (float)bestMode.h;
    
    // This is du to a strange situation. The default resolution seems to count here!
    float xScale = (float)defaultMode.h * contentAspect / (float)defaultMode.w;
    float yScale = (float)defaultMode.w * (1.0f/contentAspect) / (float)defaultMode.h;
    //float yScale = (float)1920 * (1.0f/contentAspect) / (float)1200;

    int width = bestMode.w;
    int height = bestMode.h;
    int xOffset = 0;
    int yOffset = 0;

    if (displayAspect <= contentAspect) {
        height = int((float)height * yScale);
        yOffset = (bestMode.h - height) / 2;
    }
    else {
        width = int((float)width * xScale);
        xOffset = (bestMode.w - width) / 2;
    }

    //printf("W: %d, H: %d, S:%f\n", width, height, yScale);

    SDL_GL_MakeCurrent(m_windows[windowIndex], m_glContext);    
    glViewport(xOffset, yOffset, width, height);
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