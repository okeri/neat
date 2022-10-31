wayland_sources = ['main.cc']

egl = dependency('egl')
wayland_egl = dependency('wayland-egl')
wayland_cursor = dependency('wayland-cursor')
wayland_protocols = dependency('wayland-protocols', version: '>=1.15')
wayland_protocol_path = wayland_protocols.get_pkgconfig_variable('pkgdatadir')

wayland_scanner = find_program('wayland-scanner')

wayland_scanner_code = generator(
    wayland_scanner,
    output: '@BASENAME@-protocol.c',
    arguments: ['private-code', '@INPUT@', '@OUTPUT@'])

wayland_scanner_client = generator(
    wayland_scanner,
    output: '@BASENAME@-client-protocol.h',
    arguments: ['client-header', '@INPUT@', '@OUTPUT@'])

proto = join_paths(wayland_protocol_path, 'stable/xdg-shell/xdg-shell.xml')
wayland_sources += wayland_scanner_client.process(proto)
wayland_sources += wayland_scanner_code.process(proto)

libneatwayland = static_library('neatwayland', wayland_sources,
			      include_directories: includes,
			      dependencies: [wayland_cursor,
					     wayland_egl,
					     egl])

neat_wayland = declare_dependency(link_with: libneatwayland)
deps += neat_wayland
