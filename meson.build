project('neat', ['cpp', 'c'],
    license: 'GPL3',
    default_options: ['buildtype=debug', 'cpp_std=c++17'])

cc = meson.get_compiler('cpp')
add_project_arguments(['-pedantic',
		       '-Wall',
		       '-Wextra',
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

subdir('linux')

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
			  'source/Texture.cc',
			  'source/Object.cc'],
			 include_directories: includes,
			 dependencies: deps)

neat = declare_dependency(link_with: libneat, include_directories: includes)
