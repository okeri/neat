project('neat',
        ['cpp', 'c'],
        license: 'GPL3',
        default_options: ['buildtype=debug',
                          'cpp_std=c++17',
                          'warning_level=3'])

cc = meson.get_compiler('cpp')
add_project_arguments(['-Wpedantic',
		       '-Wunreachable-code',
		       '-DGLM_CONFIG_XYZW_ONLY'],
		      language : 'cpp')

includes = include_directories('include')

stdfs = cc.find_library('stdc++fs')
freetype2 = dependency('freetype2')
glesv2 = dependency('glesv2')
glm = dependency('glm')
libpng = dependency('libpng')

deps = [freetype2,
	glesv2,
	glm,
	libpng,
	stdfs]

assimp = dependency('assimp', required: get_option('assimp'))
if assimp.found()
  add_project_arguments('-DENABLE_ASSIMP', language: 'cpp')
  deps += assimp
endif

if get_option('platform') == 'wayland'
  add_project_arguments(['-DLINUX','-DWAYLAND'], language : 'cpp')
  subdir('wayland')
elif get_option('platform') == 'x11'
  add_project_arguments(['-DLINUX', '-DX11'], language : 'cpp')
  subdir('x11')
elif get_option('platform') == 'android'
  add_project_arguments('-DANDROID', language : 'cpp')
endif

libneat = static_library('neat',
			 ['source/Asset.cc',
			  'source/Billboard.cc',
			  'source/Buffer.cc',
			  'source/Font.cc',
			  'source/GLResource.cc',
			  'source/Image.cc',
			  'source/Log.cc',
			  'source/Model.cc',
			  'source/Program.cc',
			  'source/Text.cc',
			  'source/Texture.cc'],
			 include_directories: includes,
			 dependencies: deps)

neat = declare_dependency(link_with: libneat, include_directories: includes)

if get_option('modelview').enabled()
  subdir('tools/modelview')
endif
