#include "GLHelper.h"

bool GLHelper_Init()
{
    const char *extensions = eglQueryString(eglGetCurrentDisplay(), EGL_EXTENSIONS);
    if (SDL_strstr(extensions, "EGL_EXT_image_dma_buf_import") != NULL) {
        has_EGL_EXT_image_dma_buf_import = false;
    }

    if (SDL_GL_ExtensionSupported("GL_OES_EGL_image")) {
        glEGLImageTargetTexture2DOESFunc = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
    }

    glActiveTextureARBFunc = (PFNGLACTIVETEXTUREARBPROC)SDL_GL_GetProcAddress("glActiveTextureARB");

    if (!glEGLImageTargetTexture2DOESFunc || !glActiveTextureARBFunc) {
        return false;
    }

    return true;
}