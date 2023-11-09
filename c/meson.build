project('vm-1', 'cpp')

add_project_arguments('-fpermissive', language : 'cpp')

#imgui_proj = subproject('imgui')
#imgui_dep = imgui_proj.get_variable('imgui_dep')

cpp = meson.get_compiler('cpp')
dl_dep = cpp.find_library('dl', required: true)
avcodec_dep = cpp.find_library('avcodec', required: true)
avformat_dep = cpp.find_library('avformat', required: true)
egl_dep = cpp.find_library('EGL', required: true)
gl_dep = cpp.find_library('GLESv2', required: true)

deps = [dl_dep, gl_dep, avcodec_dep, avformat_dep, egl_dep]
deps += dependency('sdl3',
  required: true,
  static: false,
  method: 'pkg-config')


executable('imgui_test', 
           'ffmpeg_test.cpp', 
           dependencies : deps
           )