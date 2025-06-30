project('i2c-test', ['c', 'cpp'], default_options: ['cpp_std=c++20'])


add_project_arguments('-fpermissive', language : 'cpp')
#add_project_arguments('-DUSE_DEV_LIB', language: 'c')

c = meson.get_compiler('cpp')
dl_dep = c.find_library('dl', required: true)

deps = []
deps += c.find_library('lgpio', required: true)
deps += c.find_library('m', required: true)  # `-lm` for math functions

sources = [ 'main.cpp' ]

#incdir = include_directories('source')
executable('i2c-test', 
           sources, 
           dependencies: deps,
 #          include_directories: incdir 
           )