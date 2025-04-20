# project('vm-1', 'cpp')
project('vm-1', ['c', 'cpp'], default_options: ['cpp_std=c++23'])


add_project_arguments('-fpermissive', language : 'cpp')
add_project_arguments('-DUSE_DEV_LIB', language: 'c')


imgui_proj = subproject('imgui')
imgui_dep = imgui_proj.get_variable('imgui_dep')

c = meson.get_compiler('cpp')
dl_dep = c.find_library('dl', required: true)

avcodec_dep = c.find_library('avcodec', required: true)
avformat_dep = c.find_library('avformat', required: true)
avfilter_dep = c.find_library('avfilter', required: true)
avutil_dep = c.find_library('avutil', required: true)
swscale_dep = c.find_library('swscale', required: true)

egl_dep = c.find_library('EGL', required: true)
gl_dep = c.find_library('GLESv2', required: true)
drm_dep = c.find_library('drm', required: true)

deps = [imgui_dep, avcodec_dep, avfilter_dep, avutil_dep, avformat_dep, swscale_dep, dl_dep, gl_dep, egl_dep, drm_dep]

deps += dependency('sdl3',
  required: true,
  static: false,
  method: 'pkg-config')

deps += c.find_library('SDL3_image', required: true)
deps += c.find_library('SDL3_test', required: true)
deps += c.find_library('unwind', required: true)

deps += c.find_library('lgpio', required: true)
deps += c.find_library('m', required: true)  # `-lm` for math functions

sources = [ 'main.cpp', 
            'source/FileAssignmentWidget.cpp', 
            'source/CameraRenderer.cpp', 
            'source/CameraPlayer.cpp', 
            'source/PlaneRenderer.cpp', 
            'source/Shader.cpp', 
            'source/VideoPlayer.cpp', 
            'source/VideoPlane.cpp',
            'source/GLHelper.cpp',
            'source/MediaController.cpp',
            'source/V4L2Controller.cpp',
            'source/CameraController.cpp',
            'source/menu/MenuSystem.cpp',
            'source/OledController.cpp',
            'source/OledUiRenderer.cpp',
            'source/oled/DEV_Config.c',  
            'source/oled/GUI_BMPfile.c',  
            'source/oled/GUI_Paint.c',  
            'source/oled/OLED_1in5_rgb.c'
          ]

incdir = include_directories('source', 'source/menu')
executable('vm1', 
           sources, 
           dependencies: deps,
           include_directories: incdir 
           )