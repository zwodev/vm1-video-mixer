
# https://github.com/Karsteski/imgui-meson-project/blob/master/subprojects/imgui-1.81/meson.build

project('imgui', 'cpp',
    version: 'docking',
    license: 'MIT',
)

include_dirs = include_directories('./imgui', 'imgui/backends')
sources = ['imgui/imgui_demo.cpp', 'imgui/imgui_draw.cpp', 'imgui/imgui_tables.cpp', 'imgui/imgui_widgets.cpp', 'imgui/imgui.cpp']

cpp = meson.get_compiler('cpp')
gl_dep = cpp.find_library('GLESv2', required: true)
dl_dep = cpp.find_library('dl', required: true)
dependencies = [gl_dep, dl_dep]

# renderer backends
if get_option('dx9').enabled()
    dx9_dep = cpp.find_library('d3d9', required: get_option('dx9'))
    if dx9_dep.found()
        sources += 'backends/imgui_impl_dx9.cpp'
        dependencies += dx9_dep
    endif
endif
if get_option('dx10').enabled()
    dx10_dep = cpp.find_library('d3d10', required: get_option('dx10'))
    if dx10_dep.found()
        sources += 'imgui/backends/imgui_impl_dx10.cpp'
        dependencies += dx10_dep
    endif
endif
if get_option('dx11').enabled()
    dx11_dep = cpp.find_library('d3d11', required: get_option('dx11'))
    if dx11_dep.found()
        sources += 'imgui/backends/imgui_impl_dx11.cpp'
        dependencies += dx11_dep
    endif
endif

if get_option('dx12').enabled()
    dx12_dep = cpp.find_library('d3d12', required: get_option('dx12'))
    if dx12_dep.found()
        sources += 'imgui/backends/imgui_impl_dx12.cpp'
        dependencies += dx12_dep
    endif
endif

if get_option('metal').enabled()
    metal_dep = cpp.find_library('metal', required: get_option('metal'))
    if get_option('metal').enabled()
        sources += 'imgui/backends/imgui_impl_metal.mm'
        dependencies += metal_dep
    endif
endif

if get_option('opengl').enabled()
    if get_option('opengl').enabled()
        #We don't need to find any libraries (for now), since they are not used in the cpp file
        sources += 'imgui/backends/imgui_impl_opengl3.cpp'
    endif
endif

if get_option('vulkan').enabled()
    vulkan_dep = dependency('vulkan', required: get_option('vulkan'))
    if vulkan_dep.found()
        sources += 'imgui/backends/imgui_impl_vulkan.cpp'
        dependencies += vulkan_dep
    endif
endif

# platform backends
if get_option('glfw').enabled()
    glfw_dep = dependency('glfw3', required: get_option('glfw'))
    if glfw_dep.found()
        sources += 'imgui/backends/imgui_impl_glfw.cpp'
        dependencies += glfw_dep
    endif
endif

if get_option('sdl2').enabled()
    sdl2_dep = dependency('sdl2', required: get_option('sdl2'))
    if sdl2_dep.found()
        sources += 'imgui/backends/imgui_impl_sdl2.cpp'
        dependencies += sdl2_dep
    endif
endif

if get_option('sdl3').enabled()
    sdl3_dep = dependency('sdl3', required: get_option('sdl3'))
    if sdl3_dep.found()
        sources += 'imgui/backends/imgui_impl_sdl3.cpp'
        dependencies += sdl3_dep
    endif
endif

if get_option('osx').enabled() or (get_option('osx').auto() and target_machine.system() == 'darwin')
    sources += 'imgui/backends/imgui_impl_osx.mm'
endif
if get_option('win').enabled() or (get_option('win').auto() and target_machine.system() == 'windows')
    sources += 'imgui/backends/imgui_impl_win32.cpp'
endif

# frameworks

if get_option('allegro5').enabled()
    allegro5_dep = dependency('allegro5', required: get_option('allegro5'))
    if allegro5_dep.found()
        sources += 'imgui/backends/imgui_impl_allegro5.cpp'
        dependencies += allegro5_dep
    endif
endif

if get_option('marmalade').enabled()
    marmalade_dep = cpp.find_library('marmalade', required: get_option('marmalade'))
    if marmalade_dep.found()
        sources += 'imgui/backends/imgui_impl_marmalade.cpp'
        dependencies += marmalade_dep
    endif
endif

imgui = library('imgui',
    sources,
    dependencies: dependencies,
    include_directories: include_dirs,
)

imgui_dep = declare_dependency(include_directories: include_dirs, link_with: imgui)