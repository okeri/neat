x11_sources = ['main.cc']

libneatx11 = static_library('neatx11', x11_sources,
			      include_directories: includes,
			      dependencies: [dependency('x11'), dependency('glx')])

neat_x11 = declare_dependency(link_with: libneatx11)
deps += neat_x11
