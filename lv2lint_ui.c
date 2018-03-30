/*
 * Copyright (c) 2016-2017 Hanspeter Portner (dev@open-music-kontrollers.ch)
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the Artistic License 2.0 as published by
 * The Perl Foundation.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License 2.0 for more details.
 *
 * You should have received a copy of the Artistic License 2.0
 * along the source as a COPYING file. If not, obtain it from
 * http://www.perlfoundation.org/artistic_license_2_0.
 */

#include <math.h>
#include <string.h>

#ifdef _WIN32
#	include <windows.h>
#	define dlopen(path, flags) LoadLibrary(path)
#	define dlclose(lib) FreeLibrary((HMODULE)lib)
#	define inline __inline
#	define snprintf _snprintf
static inline char* dlerror(void) { return "Unknown error"; }
#else
#	include <dlfcn.h>
#endif

#include <lv2lint.h>

#include <lv2/lv2plug.in/ns/ext/instance-access/instance-access.h>
#include <lv2/lv2plug.in/ns/ext/data-access/data-access.h>

#ifdef ENABLE_ELF_TESTS
static const ret_t *
_test_symbols(app_t *app)
{
	static const ret_t ret_symbols = {
		.lnt = LINT_FAIL,
		.msg = "binary exports superfluous globally visible symbols",
		.uri = LV2_CORE__binary,
		.dsc = "plugin UI binaries must not export any globally visible symbols\n"
		       "but lv2ui_descriptor. You may well have forgotten to compile\n"
					 "with -fvisibility=hidden."}; //FIXME actually print me

	const ret_t *ret = NULL;

	const LilvNode* node = lilv_ui_get_binary_uri(app->ui);
	if(node && lilv_node_is_uri(node))
	{
		const char *uri = lilv_node_as_uri(node);
		if(uri)
		{
			char *path = lilv_file_uri_parse(uri, NULL);
			if(path)
			{
				if(!test_visibility(path, "lv2ui_descriptor"))
				{
					ret = &ret_symbols;
				}

				lilv_free(path);
			}
		}
	}

	return ret;
}
#endif

static const ret_t *
_test_instance_access(app_t *app)
{
	static const ret_t ret_instance_access_discouraged = {
		LINT_WARN, "usage of instance-access is highly discouraged", LV2_INSTANCE_ACCESS_URI, NULL};

	const ret_t *ret = NULL;

	const LilvNode *uri = lilv_ui_get_uri(app->ui);

	const bool needs_instance_access = lilv_world_ask(app->world, uri,
		app->uris.lv2_requiredFeature, app->uris.instance_access);

	if(needs_instance_access)
	{
		ret = &ret_instance_access_discouraged;
	}

	return ret;
}

static const ret_t *
_test_data_access(app_t *app)
{
	static const ret_t ret_data_access_discouraged = {
		LINT_WARN, "usage of data-access is highly discouraged", LV2_DATA_ACCESS_URI, NULL};

	const ret_t *ret = NULL;

	const LilvNode *uri = lilv_ui_get_uri(app->ui);

	const bool needs_data_access = lilv_world_ask(app->world, uri,
		app->uris.lv2_requiredFeature, app->uris.data_access);

	if(needs_data_access)
	{
		ret = &ret_data_access_discouraged;
	}

	return ret;
}

static const ret_t *
_test_mixed(app_t *app)
{
	static const ret_t ret_mixed_discouraged = {
		LINT_WARN, "mixing DSP and UI code in same binary is discouraged", LV2_UI_PREFIX, NULL};

	const ret_t *ret = NULL;

	const LilvNode *library_uri = lilv_plugin_get_library_uri(app->plugin);

	const LilvNode *ui_library_uri = lilv_ui_get_binary_uri(app->ui);
	if(ui_library_uri && lilv_node_equals(library_uri, ui_library_uri))
	{
		ret = &ret_mixed_discouraged;
	}

	return ret;
}

#if 0 //FIXME
static const ret_t *
_test_binary(app_t *app)
{
	static const ret_t ret_binary_deprecated = {
		LINT_FAIL, "ui:binary is deprecated, use lv2:binary instead", LV2_UI__binary, NULL};

	const ret_t *ret = NULL;

	const bool has_ui_binary = lilv_world_ask(app->world,
		lilv_ui_get_uri(app->ui), app->uris.ui_binary, NULL);

	if(has_ui_binary)
	{
		ret = &ret_binary_deprecated;
	}

	return ret;
}
#endif

static const ret_t *
_test_resident(app_t *app)
{
	static const ret_t ret_resident_deprecated = {
		LINT_FAIL, "ui:makeSONameResident is deprecated", LV2_UI_PREFIX"makeSONameResident", NULL};

	const ret_t *ret = NULL;

	const bool has_ui_resident = lilv_world_ask(app->world,
		lilv_ui_get_uri(app->ui), app->uris.ui_makeSONameResident, NULL);

	if(has_ui_resident)
	{
		ret = &ret_resident_deprecated;
	}

	return ret;
}

static const ret_t *
_test_idle_interface(app_t *app)
{
	static const ret_t ret_idle_feature_missing = {
		LINT_WARN, "lv2:feature ui:idleInterface missing", LV2_UI__idleInterface, NULL},
	ret_idle_extension_missing = {
		LINT_FAIL, "lv2:extensionData ui:idleInterface missing", LV2_UI__idleInterface, NULL},
	ret_idle_extension_not_returned = {
		LINT_FAIL, "ui:idleInterface not returned by 'extention_data'", LV2_UI__idleInterface, NULL};

	const ret_t *ret = NULL;

	const bool has_idle_feature = lilv_world_ask(app->world,
			lilv_ui_get_uri(app->ui), app->uris.lv2_optionalFeature, app->uris.ui_idleInterface)
		|| lilv_world_ask(app->world,
			lilv_ui_get_uri(app->ui), app->uris.lv2_requiredFeature, app->uris.ui_idleInterface);
	const bool has_idle_extension = lilv_world_ask(app->world,
		lilv_ui_get_uri(app->ui), app->uris.lv2_extensionData, app->uris.ui_idleInterface);

	if( (has_idle_extension || app->ui_idle_iface) && !has_idle_feature)
	{
		ret = &ret_idle_feature_missing;
	}
	else if( (has_idle_feature || app->ui_idle_iface) && !has_idle_extension)
	{
		ret = &ret_idle_extension_missing;
	}
	else if( (has_idle_extension || has_idle_feature) && !app->ui_idle_iface)
	{
		ret = &ret_idle_extension_not_returned;
	}

	return ret;
}

static const ret_t *
_test_show_interface(app_t *app)
{
	static const ret_t ret_show_extension_missing =	{
		LINT_FAIL, "lv2:extensionData ui:showInterface missing", LV2_UI__showInterface, NULL},
	ret_show_extension_not_returned = {
		LINT_FAIL, "ui:showInterface not returned by 'extention_data'", LV2_UI__showInterface, NULL};

	const ret_t *ret = NULL;

	const bool has_show_extension = lilv_world_ask(app->world,
		lilv_ui_get_uri(app->ui), app->uris.lv2_extensionData, app->uris.ui_showInterface);

	if(app->ui_show_iface && !has_show_extension)
	{
		ret = &ret_show_extension_missing;
	}
	else if(has_show_extension && !app->ui_show_iface)
	{
		ret = &ret_show_extension_not_returned;
	}

	return ret;
}

static const ret_t *
_test_resize_interface(app_t *app)
{
	static const ret_t ret_resize_missing = {
		LINT_FAIL, "lv2:extensionData ui:resize missing", LV2_UI__resize, NULL},
	ret_resize_not_returned = {
		LINT_FAIL, "ui:resize not returned by 'extention_data'", LV2_UI__resize, NULL};

	const ret_t *ret = NULL;

	const bool has_resize_extension = lilv_world_ask(app->world,
		lilv_ui_get_uri(app->ui), app->uris.lv2_extensionData, app->uris.ui_resize);

	if(app->ui_resize_iface && !has_resize_extension)
	{
		ret = &ret_resize_missing;
	}
	else if(has_resize_extension && !app->ui_resize_iface)
	{
		ret = &ret_resize_not_returned;
	}

	return ret;
}

static const ret_t *
_test_toolkit(app_t *app)
{
	static const ret_t ret_toolkit_invalid = {
		LINT_FAIL, "UI toolkit not given", LV2_UI__ui, NULL},
	ret_toolkit_show_interface = {
		LINT_WARN, "usage of external UI is discouraged", LV2_UI__showInterface, NULL},
	ret_toolkit_external = {
		LINT_WARN, "usage of external UI is discouraged", LV2_EXTERNAL_UI__Widget, NULL},
	ret_toolkit_unknown = {
		LINT_FAIL, "UI toolkit <%s> unkown", LV2_UI__ui, NULL},
	ret_toolkit_non_native = {
		LINT_WARN, "usage of non-native toolkit <%s> is dicouraged", LV2_UI__ui, NULL};

	const ret_t *ret = NULL;

	const LilvNode *ui_uri_node = lilv_ui_get_uri(app->ui);
	LilvNode *ui_class_node = lilv_world_get(app->world, ui_uri_node, app->uris.rdf_type, NULL);
	LilvNodes *ui_class_nodes = lilv_world_find_nodes(app->world, NULL, app->uris.rdfs_subClassOf, app->uris.ui_UI);

#if defined(_WIN32)
	const bool is_windows_ui = lilv_ui_is_a(app->ui, app->uris.ui_WindowsUI);
#elif defined(__APPLE__)
	const bool is_cocoa_ui = lilv_ui_is_a(app->ui, app->uris.ui_CocoaUI);
#else
	const bool is_x11_ui = lilv_ui_is_a(app->ui, app->uris.ui_X11UI);
#endif

	const bool has_show_extension = lilv_world_ask(app->world,
		lilv_ui_get_uri(app->ui), app->uris.lv2_extensionData, app->uris.ui_showInterface);
	const bool is_external = lilv_node_equals(ui_class_node, app->uris.ext_Widget);

	bool is_known = false;
	if(ui_class_node && ui_class_nodes)
	{
		if(lilv_nodes_contains(ui_class_nodes, ui_class_node))
			is_known = true;
	}

	if(!ui_class_node)
	{
		ret = &ret_toolkit_invalid;
	}
	else if(app->ui_show_iface || has_show_extension)
	{
		ret = &ret_toolkit_show_interface;
	}
	else if(is_external)
	{
		ret = &ret_toolkit_external;
	}
	else if(!is_known)
	{
		*app->urn = strdup(lilv_node_as_uri(ui_class_node));
		ret = &ret_toolkit_unknown;
	}
#if defined(_WIN32)
	else if(!is_windows_ui)
#elif defined(__APPLE__)
	else if(!is_cocoa_ui)
#else
	else if(!is_x11_ui)
#endif
	{
		*app->urn = strdup(lilv_node_as_uri(ui_class_node));
		ret = &ret_toolkit_non_native;
	}

	if(ui_class_node)
		lilv_node_free(ui_class_node);
	if(ui_class_nodes)
		lilv_nodes_free(ui_class_nodes);

	return ret;
}

#ifdef ENABLE_ONLINE_TESTS

static const ret_t *
_test_ui_url(app_t *app)
{
	static const ret_t ret_ui_url_not_existing = {
		LINT_WARN, "UI Web URL does not exist", LV2_UI__UI, NULL};

	const ret_t *ret = NULL;

	const char *uri = lilv_node_as_uri(lilv_ui_get_uri(app->ui));

	if(is_url(uri))
	{
		const bool url_exists = !app->online || test_url(app, uri);

		if(!url_exists)
		{
			ret = &ret_ui_url_not_existing;
		}
	}

	return ret;
}
#endif

static const test_t tests [] = {
#ifdef ENABLE_ELF_TESTS
	{"Symbols",          _test_symbols},
#endif
	{"Instance Access",  _test_instance_access},
	{"Data Access",      _test_data_access},
	{"Mixed DSP/UI",     _test_mixed},
	//{"UI Binary",        _test_binary}, FIXME lilv does not support lv2:binary for UIs, yet
	{"UI SO Name",       _test_resident},
	{"Idle Interface",   _test_idle_interface},
	{"Show Interface",   _test_show_interface},
	{"Resize Interface", _test_resize_interface},
	{"Toolkit",          _test_toolkit},
#ifdef ENABLE_ONLINE_TESTS
	{"UI URL",           _test_ui_url},
#endif
};

static const unsigned tests_n = sizeof(tests) / sizeof(test_t);

bool
test_ui(app_t *app)
{
	bool flag = true;
	bool msg = false;
	res_t *rets = alloca(tests_n * sizeof(res_t));
	if(!rets)
		return flag;

	void *lib = NULL;
	const LV2UI_Descriptor *descriptor = NULL;

	const LilvNode *ui_uri_node = lilv_ui_get_uri(app->ui);
	const LilvNode *ui_binary_node = lilv_ui_get_binary_uri(app->ui);

	const char *ui_uri = lilv_node_as_uri(ui_uri_node);
	const char *ui_binary_uri = lilv_node_as_uri(ui_binary_node);
	char *ui_binary_path = lilv_file_uri_parse(ui_binary_uri, NULL);

	dlerror();

	lib = dlopen(ui_binary_path, RTLD_NOW);
	if(!lib)
	{
		fprintf(stderr, "Unable to open UI library %s (%s)\n", ui_binary_path, dlerror());
		goto jump;
	}

	// Get discovery function
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#ifdef _WIN32
	LV2UI_DescriptorFunction df = (LV2UI_DescriptorFunction)GetProcAddress(lib, "lv2ui_descriptor");
#else
	LV2UI_DescriptorFunction df = (LV2UI_DescriptorFunction)dlsym(lib, "lv2ui_descriptor");
#endif
#pragma GCC diagnostic pop
	if(!df)
	{
		fprintf(stderr, "Broken LV2 UI %s (no lv2ui_descriptor symbol found)\n", ui_binary_path);
		dlclose(lib);
		goto jump;
	}

	// Get UI descriptor
	for(uint32_t i = 0; ; i++)
	{
		const LV2UI_Descriptor *ld = df(i);
		if(!ld)
		{
			break; // sentinel
		}
		else if(!strcmp(ld->URI, ui_uri))
		{
			descriptor = ld;
			break;
		}
	}

	if(!descriptor)
	{
		fprintf(stderr, "Failed to find descriptor for <%s> in %s\n", ui_uri, ui_binary_path);
		dlclose(lib);
		goto jump;
	}

	app->ui_idle_iface = descriptor->extension_data
		? descriptor->extension_data(LV2_UI__idleInterface)
		: NULL;
	app->ui_show_iface = descriptor->extension_data
		? descriptor->extension_data(LV2_UI__showInterface)
		: NULL;
	app->ui_resize_iface = descriptor->extension_data
		? descriptor->extension_data(LV2_UI__resize)
		: NULL;

	for(unsigned i=0; i<tests_n; i++)
	{
		const test_t *test = &tests[i];
		res_t *res = &rets[i];

		res->urn = NULL;
		app->urn = &res->urn;
		res->ret = test->cb(app);
		if(res->ret && (res->ret->lnt & app->show) )
			msg = true;
	}

	app->ui_idle_iface = NULL;
	app->ui_show_iface = NULL;
	app->ui_resize_iface = NULL;

	const bool show_passes = LINT_PASS & app->show;

	if(msg || show_passes)
	{
		lv2lint_printf(app, "  %s<%s>%s\n",
			colors[app->atty][ANSI_COLOR_BOLD],
			lilv_node_as_uri(lilv_ui_get_uri(app->ui)),
			colors[app->atty][ANSI_COLOR_RESET]);

		for(unsigned i=0; i<tests_n; i++)
		{
			const test_t *test = &tests[i];
			res_t *res = &rets[i];

			lv2lint_report(app, test, res, show_passes, &flag);
		}
	}

jump:
	if(ui_binary_path)
	{
		lilv_free(ui_binary_path);
	}

	if(lib)
	{
		dlclose(lib);
		lib = NULL;
	}

	return flag;
}
