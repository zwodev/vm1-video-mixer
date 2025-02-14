#include <SDL3/SDL.h>
#include <SDL3/SDL_opengles2.h>
#include <GLES3/gl31.h>
#include <stdio.h>

#define WIDTH 512
#define HEIGHT 512

const char* computeShaderSource = 
    "#version 310 es\n"
    "layout(local_size_x = 16, local_size_y = 16) in;\n"
    "layout(rgba8, binding = 0) uniform writeonly highp image2D outputImage;\n"
    "void main() {\n"
    "    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);\n"
    "    vec4 white = vec4(1.0, 1.0, 1.0, 1.0);\n"
    "    imageStore(outputImage, pixelCoords, white);\n"
    "}\n";

GLuint createShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    return shader;
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Compute Shader Example", WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        printf("OpenGL context could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    GLuint computeShader = createShader(GL_COMPUTE_SHADER, computeShaderSource);
    GLuint computeProgram = glCreateProgram();
    glAttachShader(computeProgram, computeShader);
    glLinkProgram(computeProgram);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, WIDTH, HEIGHT);

    glUseProgram(computeProgram);
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    glDispatchCompute(WIDTH / 16, HEIGHT / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, WIDTH, HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the texture to the screen
    // (You'd need to set up a simple vertex and fragment shader for this)

    SDL_GL_SwapWindow(window);

    SDL_Delay(3000);  // Wait for 3 seconds

    glDeleteTextures(1, &texture);
    glDeleteProgram(computeProgram);
    glDeleteShader(computeShader);
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
