// Dear ImGui: standalone example application for SDL3 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

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

#include "source/PlaneRenderer.h"
#include "source/VideoPlayer.h"
#include "source/VideoPlane.h"
#include "source/CameraRenderer.h"

// Main code
int main(int, char**)
{
    // Setup SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    // const char* glsl_version = "#version 130";
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    const char* glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif

    int num_displays;
    SDL_DisplayID *displays = SDL_GetDisplays(&num_displays);
    SDL_Log("Found %d display(s)", num_displays);
    for (int i = 0; i < num_displays; ++i) {
        SDL_Log("Display ID for %d: %d", i, displays[i]);
    }

    SDL_free(displays);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_PropertiesID props = SDL_CreateProperties();
    if(props == 0) {
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
    
    std::vector<SDL_Window*> windows;
    for (int i = 0; i < num_displays; ++i) {
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, 1920 * i);
        //SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_MOUSE_GRABBED_BOOLEAN, (i == 0));
        
        SDL_Window* window = SDL_CreateWindowWithProperties(props);
        if (window == nullptr)
        {
            printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
            return -1;
        }
        windows.push_back(window);
    }

    if(windows.size() < 1) {
        SDL_Log("Unable to create an SDL window!");
        return 0;
    }
    
    SDL_GLContext gl_context = SDL_GL_CreateContext(windows[0]);
    if (gl_context == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_SetSwapInterval(1); // Enable vsync
    for (int i = 0; i < windows.size(); ++i) { 
        SDL_ShowWindow(windows[i]);
    }

    // if (windows.size() > 1 ) {
    //     SDL_SetWindowFocusable(windows[0], false);
    // }
    //SDL_GL_MakeCurrent(windows[0], gl_context);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(windows[0], gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Video players
    VideoPlane videoPlane0;
    videoPlane0.players()[0]->open("../videos/jellyfish_1080_10s_h265.mp4");
    //videoPlane0.players()[0]->open("rtsp://192.168.178.23:8554/test");
    videoPlane0.players()[0]->play();
    //videoPlane0.players()[1]->open("../videos/jellyfish_1080_10s_h265.mp4");
    videoPlane0.players()[1]->open("../videos/bigbuckbunny_1080_10s_h265.mp4");
    videoPlane0.players()[1]->play();
    

    VideoPlane videoPlane1;
    videoPlane1.players()[0]->open("../videos/jellyfish_1080_10s_h265.mp4");
    videoPlane1.players()[0]->play();
    //videoPlane1.players()[1]->open("../videos/jellyfish_1080_10s_h265.mp4");
    videoPlane1.players()[1]->open("../videos/bigbuckbunny_1080_10s_h265.mp4");
    videoPlane1.players()[1]->play();

    // Camera
    CameraRenderer cameraRenderer0;
    cameraRenderer0.start();


    //ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    //ImGui_ImplSDL3_Data* bd = ImGui_ImplSDL3_GetBackendData();
    //io.BackendPlatformUserData->MouseWindowID = windows[0];
    SDL_RaiseWindow(windows[0]);
    //SDL_Mouse *mouse = SDL_GetMouse();
    
    float mixValue = 0.0f;
    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // if (event.type == SDL_EVENT_MOUSE_MOTION) {
            //     event.motion.windowID = SDL_GetWindowID(windows[0]);
            // }
            // else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
            //     event.wheel.windowID = SDL_GetWindowID(windows[0]);
            // }
            // else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            //     event.button.windowID = SDL_GetWindowID(windows[0]);
            // }
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                done = true;
            }
            else if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(windows[0])) {
                done = true;
            }
        }
        if (SDL_GetWindowFlags(windows[0]) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();


        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("Mix Value", &mixValue, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }


        // Rendering window 0
        SDL_GL_MakeCurrent(windows[0], gl_context);
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render video content
        videoPlane0.update(mixValue);
        cameraRenderer0.update();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(windows[0]);

        // Rendering window 1
        if (windows.size() > 1) {
            SDL_GL_MakeCurrent(windows[1], gl_context);
            glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
            glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);

            // Render video content
            videoPlane1.update(mixValue);

            SDL_GL_SwapWindow(windows[1]);
        }
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    for (int i = 0; i < windows.size(); ++i) {
        SDL_DestroyWindow(windows[i]);
    }
    SDL_Quit();

    return 0;
}