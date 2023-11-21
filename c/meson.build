project('vm-1', 'cpp')

add_project_arguments('-fpermissive', language : 'cpp')

#imgui_proj = subproject('imgui')
#imgui_dep = imgui_proj.get_variable('imgui_dep')

c = meson.get_compiler('cpp')
dl_dep = c.find_library('dl', required: true)

avcodec_dep = c.find_library('avcodec', required: true)
avformat_dep = c.find_library('avformat', required: true)
avfilter_dep = c.find_library('avfilter', required: true)
avutil_dep = c.find_library('avutil', required: true)
swscale_dep = c.find_library('swscale', required: true)

egl_dep = c.find_library('EGL', required: true)
gl_dep = c.find_library('GLESv2', required: true)

deps = [avcodec_dep, avfilter_dep, avutil_dep, avformat_dep, swscale_dep, dl_dep, gl_dep, egl_dep]

deps += dependency('sdl3',
  required: true,
  static: false,
  method: 'pkg-config')

deps += c.find_library('SDL3_image', required: true)
deps += c.find_library('SDL3_test', required: true)
deps += c.find_library('unwind', required: true)

sources = ['sdl_test.cpp', 'source/PlaneRenderer.cpp', 'source/Shader.cpp']

executable('imgui_test', 
           sources, 
           dependencies : deps
           )