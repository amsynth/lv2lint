project('lv2lint', 'c', default_options : [
	'buildtype=release',
	'warning_level=3',
	'werror=false',
	'b_lto=false',
	'c_std=gnu11'])

online_tests = get_option('online-tests')
elf_tests = get_option('elf-tests')
x11_tests = get_option('x11-tests')

version = run_command('cat', 'VERSION').stdout().strip()

add_project_arguments('-DLV2LINT_VERSION="'+version+'"', language : 'c')
add_project_arguments('-D_GNU_SOURCE', language : 'c')

conf_data = configuration_data()
cc = meson.get_compiler('c')

m_dep = cc.find_library('m')
lv2_dep = dependency('lv2', version : '>=1.18.0')
lilv_dep = dependency('lilv-0', version : '>=0.24.0',
	static : meson.is_cross_build() and false) #FIXME
curl_dep = dependency('libcurl', required: online_tests)
elf_dep = dependency('libelf', required: elf_tests)
x11_dep = dependency('x11', version : '>=1.6.0', required : x11_tests)
xext_dep = dependency('xext', version : '>=1.3.0', required : x11_tests)
xcb_dep = dependency('xcb', version : '>=1.12', required : x11_tests)
xcbicccm_dep = dependency('xcb-icccm', version : '>=0.4.0', required : x11_tests)
xcbxrm_dep = dependency('xcb-xrm', version : '>=1.0', required : x11_tests)
	
deps = [m_dep, lv2_dep, lilv_dep, curl_dep, elf_dep, x11_dep, xext_dep, xcb_dep, xcbicccm_dep, xcbxrm_dep]

mapper_inc = include_directories('mapper.lv2')
incs = [mapper_inc]

if online_tests.enabled()
	add_project_arguments('-DENABLE_ONLINE_TESTS', language : 'c')
	conf_data.set('ONLINE_TESTS', '')
else
	conf_data.set('ONLINE_TESTS', './')
endif

if cc.has_header('fnmatch.h') and cc.has_function('fnmatch')
	add_project_arguments('-DHAS_FNMATCH', language : 'c')
endif

if elf_tests.enabled()
	add_project_arguments('-DENABLE_ELF_TESTS', language : 'c')
	conf_data.set('ELF_TESTS', '')
else
	conf_data.set('ELF_TESTS', './')
endif

srcs = [
	'lv2lint.c',
	'lv2lint_plugin.c',
	'lv2lint_port.c',
	'lv2lint_parameter.c',
	'lv2lint_ui.c'
]

if x11_tests.enabled()
	add_project_arguments('-DENABLE_X11_TESTS', language : 'c')
	conf_data.set('X11_TESTS', '')
	srcs += 'lv2lint_x11.c'
else
	conf_data.set('X11_TESTS', './')
endif

executable('lv2lint', srcs,
	include_directories : incs,
	dependencies : deps,
	install : true)

configure_file(input : 'lv2lint.1.in', output : 'lv2lint.1',
	configuration : conf_data)

install_man(join_paths(meson.current_build_dir(), 'lv2lint.1'))
