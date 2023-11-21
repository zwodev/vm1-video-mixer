/*
 * g++ main.cpp shader.cpp `pkg-config --cflags --libs sdl2 glesv2`
 */

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengles2.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "source/PlaneRenderer.h"
#include "source/Shader.h"

using namespace std;

const unsigned int DISP_WIDTH = 640;
const unsigned int DISP_HEIGHT = 480;


int SDL_main(int argc, char *args[]) {
	
	// The window
	SDL_Window *window = NULL;
	
	// The OpenGL context
	SDL_GLContext context = NULL;
	
	// IMPORTANT! These sets must go BEFORE SDL_Init
	// Request OpenGL ES 3.0
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	
	// Want double-buffering
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	
	// Init SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_Log("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		cout << "SDL could not initialize! SDL_Error:\n";
		return EXIT_FAILURE;
	}
	
	// Setup the exit hook
	atexit(SDL_Quit);
	

	
	// Create the window
	window = SDL_CreateWindow("GLES3+SDL2 Tutorial", DISP_WIDTH, DISP_HEIGHT,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
		if(!window){
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error",
				"Couldn't create the main window.", NULL);
			cout << "Couldn't create the main window.\n";
			return EXIT_FAILURE;
			}
		
		context = SDL_GL_CreateContext(window);
		if(!context) {
				SDL_ShowSimpleMessageBox(
					SDL_MESSAGEBOX_ERROR, "Error",
					"Couldn't create an OpenGL context.", NULL);
				cout << "Couldn't create an OpenGL context.\n";
				return EXIT_FAILURE;
			}


	PlaneRenderer planeRenderer;

	// Clear to black
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	// Update the window
	SDL_GL_SwapWindow(window);

	
	// Wait for the user to quit
	bool quit = false;
	while (!quit) {
		SDL_Event event;
		if (SDL_WaitEvent(&event) != 0) {
				if (event.type == SDL_EVENT_QUIT) {
					// User wants to quit
					quit = true;
				}
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		planeRenderer.update();
		SDL_GL_SwapWindow(window); 
	}
	
	return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
		return SDL_main(argc, argv);
	}