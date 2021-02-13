/*
 * Copyright (c) 2016-2021 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#include <assert.h>

#include <lv2lint.h>

#include <lv2/lv2plug.in/ns/ext/log/log.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/parameters/parameters.h>
#include <lv2/lv2plug.in/ns/ext/instance-access/instance-access.h>
#include <lv2/lv2plug.in/ns/ext/data-access/data-access.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

static void
_write_function(LV2UI_Controller controller __unused, uint32_t index __unused,
	uint32_t size __unused, uint32_t protocol __unused, const void *buf __unused)
{
	// nothing to do
}

static uint32_t
_port_subscribe(LV2UI_Feature_Handle handle __unused, uint32_t index __unused,
	uint32_t protocol __unused, const LV2_Feature *const *features __unused)
{
	return 0;
}

static uint32_t
_port_unsubscribe(LV2UI_Feature_Handle handle __unused, uint32_t index __unused,
	uint32_t protocol __unused, const LV2_Feature *const *features __unused)
{
	return 0;
}

static void
_touch(LV2UI_Feature_Handle handle __unused, uint32_t index __unused,
	bool grabbed __unused)
{
	// nothing to do
}

static uint32_t
_port_index(LV2UI_Feature_Handle handle, const char *symbol)
{
	app_t *app = handle;
	uint32_t index = LV2UI_INVALID_PORT_INDEX;

	LilvNode *symbol_uri = lilv_new_string(app->world, symbol);
	if(symbol_uri)
	{
		const LilvPort *port = lilv_plugin_get_port_by_symbol(app->plugin, symbol_uri);

		if(port)
		{
			index = lilv_port_get_index(app->plugin, port);
		}

		lilv_node_free(symbol_uri);
	}

	return index;
}

static LV2UI_Request_Value_Status
_request_value(LV2UI_Feature_Handle handle __unused, LV2_URID key __unused,
	LV2_URID type __unused, const LV2_Feature* const* features __unused)
{
	return LV2UI_REQUEST_VALUE_SUCCESS;
}

static int
_resize(LV2UI_Feature_Handle handle __unused, int width __unused,
	int height __unused)
{
	return 0;
}

static const ret_t *
_test_ui_instantiation(app_t *app)
{
	static const ret_t ret_instantiation = {
		.lnt = LINT_FAIL,
		.msg = "failed to instantiate",
		.uri = LV2_CORE_URI, //FIXME
		.dsc = "You likely have forgotten to list all lv2:requiredFeature's."
	};

	const ret_t *ret = NULL;

	if(!app->ui_instance)
	{
		ret = &ret_instantiation;
	}

	return ret;
}

static const ret_t *
_test_ui_widget(app_t *app)
{
	static const ret_t ret_widget = {
		.lnt = LINT_FAIL,
		.msg = "failed to return a valid widget",
		.uri = LV2_CORE_URI, //FIXME
		.dsc = "You likely have forgotten to return the proper XWindow ID."
	};

	const ret_t *ret = NULL;

	if(!app->ui_widget)
	{
		ret = &ret_widget;
	}

	return ret;
}

static const ret_t *
_test_ui_hints(app_t *app)
{
	static const ret_t ret_fixed_aspect = {
		.lnt = LINT_WARN,
		.msg = "widget uses fixed aspect ratio",
		.uri = LV2_CORE_URI, //FIXME
		.dsc = "Windows with fixed aspect ratio are a pain in tiling window mangers."
	},
	ret_aspect_constraints = {
		.lnt = LINT_WARN,
		.msg = "widget uses aspect ratio constraints",
		.uri = LV2_CORE_URI, //FIXME
		.dsc = "Windows with aspect ratio constraints are a pain in tiling window mangers."
	},
	ret_fixed_size  = {
		.lnt = LINT_WARN,
		.msg = "widget uses fixed size",
		.uri = LV2_CORE_URI, //FIXME
		.dsc = "Windows with fixed sizes are a pain in tiling window mangers."
	};

	const ret_t *ret = NULL;

	XSizeHints hints;
	memset(&hints, 0x1, sizeof(hints));
	long supplied = 0;

  Display *display = XOpenDisplay(NULL); // FIXME reuse existing one
	if(display)
	{
		XGetWMNormalHints(display, app->ui_widget, &hints, &supplied);

		XCloseDisplay(display);
	}

	if(supplied & PAspect)
	{
		const float ratio_min = hints.min_aspect.y && hints.min_aspect.x
			? (float)hints.min_aspect.y / hints.min_aspect.x
			: 0.f;
		const float ratio_max = hints.max_aspect.y && hints.max_aspect.x
			? (float)hints.max_aspect.y / hints.max_aspect.x
			: 0.f;

		if(ratio_min && ratio_max && (ratio_min == ratio_max) )
		{
			ret = &ret_fixed_aspect;
		}
		else if(ratio_min || ratio_max)
		{
			ret = &ret_aspect_constraints;
		}
	}

	if(supplied & (PSize | PMinSize | PMaxSize) )
	{
		if( (hints.width == hints.min_width) && (hints.width == hints.max_width)
		&&  (hints.height == hints.min_height) && (hints.height == hints.max_height) )
		{
			ret = &ret_fixed_size;
		}
	}

	return ret;
}

static const test_t tests [] = {
	{"UI Instantiation",    _test_ui_instantiation},
	{"UI Widget",           _test_ui_widget},
	{"UI Hints",            _test_ui_hints},
};

static const unsigned tests_n = sizeof(tests) / sizeof(test_t);

void
test_x11(app_t *app, bool *flag)
{
	bool msg = false;
	res_t *rets = alloca(tests_n * sizeof(res_t));
	if(!rets)
		return;

  Display *display = NULL;
	Window win = 0;

	display = XOpenDisplay(NULL);
	if(!display)
	{
		goto jump;
	}

	const int black = BlackPixel(display, DefaultScreen(display));
  win = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0,
		600, 600, 0, black, black);
	if(!win)
	{
		goto jump;
	}

	XFlush(display);

	LV2_Log_Log log = {
		.handle = app,
		.printf = log_printf,
		.vprintf = log_vprintf
	};
	LV2UI_Port_Map port_map = {
		.handle = app,
		.port_index = _port_index
	};
	LV2UI_Port_Subscribe port_subscribe = {
		.handle = app,
		.subscribe = _port_subscribe,
		.unsubscribe = _port_unsubscribe
	};
	LV2UI_Touch touch = {
		.handle = app,
		.touch = _touch
	};
	LV2UI_Request_Value request_value = {
		.handle = app,
		.request = _request_value
	};
	LV2UI_Resize host_resize = {
		.handle = app,
		.ui_resize = _resize
	};
	LV2_Extension_Data_Feature data_access = {
		.data_access = app->descriptor ? app->descriptor->extension_data : NULL
	};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	LV2_URI_Map_Feature urimap = {
		.callback_data = app->map,
		.uri_to_id = uri_to_id
	};
#pragma GCC diagnostic pop

	const LV2_URID atom_Float = app->map->map(app->map->handle, LV2_ATOM__Float);
	const LV2_URID param_sampleRate = app->map->map(app->map->handle, LV2_PARAMETERS__sampleRate);
	const LV2_URID ui_updateRate = app->map->map(app->map->handle, LV2_UI__updateRate);

	const float param_sample_rate = 48000.f;
	const float ui_update_rate = 25.f;

	const LV2_Options_Option opts_sampleRate = {
		.key = param_sampleRate,
		.size = sizeof(float),
		.type = atom_Float,
		.value = &param_sample_rate
	};
	const LV2_Options_Option opts_updateRate = {
		.key = ui_updateRate,
		.size = sizeof(float),
		.type = atom_Float,
		.value = &ui_update_rate
	};
	const LV2_Options_Option opts_sentinel = {
		.key = 0,
		.value =NULL
	};

#define MAX_OPTS 2
	LV2_Options_Option opts [MAX_OPTS];

	const LV2_Feature feat_parent = {
		.URI = LV2_UI__parent,
		.data = (void *)(uintptr_t)win
	};
	const LV2_Feature feat_map = {
		.URI = LV2_URID__map,
		.data = app->map
	};
	const LV2_Feature feat_unmap = {
		.URI = LV2_URID__unmap,
		.data = app->unmap
	};
	const LV2_Feature feat_log = {
		.URI = LV2_LOG__log,
		.data = &log
	};
	const LV2_Feature feat_port_map = {
		.URI = LV2_UI__portMap,
		.data = &port_map
	};
	const LV2_Feature feat_port_subscribe = {
		.URI = LV2_UI__portSubscribe,
		.data = &port_subscribe
	};
	const LV2_Feature feat_touch = {
		.URI = LV2_UI__touch,
		.data = &touch
	};
	const LV2_Feature feat_request_value = {
		.URI = LV2_UI__requestValue,
		.data = &request_value
	};
	const LV2_Feature feat_resize = {
		.URI = LV2_UI__resize,
		.data = &host_resize
	};
	const LV2_Feature feat_instance_access = {
		.URI = LV2_INSTANCE_ACCESS_URI,
		.data = app->instance
	};
	const LV2_Feature feat_data_access = {
		.URI = LV2_DATA_ACCESS_URI,
		.data = &data_access
	};
	const LV2_Feature feat_urimap = {
		.URI = LV2_URI_MAP_URI,
		.data = &urimap
	};
	const LV2_Feature feat_opts = {
		.URI = LV2_OPTIONS__options,
		.data = opts
	};

#define MAX_FEATURES 13
	const LV2_Feature *features [MAX_FEATURES];
	unsigned f = 0;
	{
		const LilvNode *uri = lilv_ui_get_uri(app->ui);

		LilvNodes *required_features = lilv_world_find_nodes(app->world, uri,
			app->uris.lv2_requiredFeature, NULL);
		if(required_features)
		{
			LILV_FOREACH(nodes, itr, required_features)
			{
				const LilvNode *feature = lilv_nodes_get(required_features, itr);

				if(lilv_node_equals(feature, app->uris.urid_map))
				{
					features[f++] = &feat_map;
				}
				else if(lilv_node_equals(feature, app->uris.urid_unmap))
				{
					features[f++] = &feat_unmap;
				}
				else if(lilv_node_equals(feature, app->uris.ui_parent))
				{
					features[f++] = &feat_parent;
				}
				else if(lilv_node_equals(feature, app->uris.log_log))
				{
					features[f++] = &feat_log;
				}
				else if(lilv_node_equals(feature, app->uris.ui_portMap))
				{
					features[f++] = &feat_port_map;
				}
				else if(lilv_node_equals(feature, app->uris.ui_portSubscribe))
				{
					features[f++] = &feat_port_subscribe;
				}
				else if(lilv_node_equals(feature, app->uris.ui_touch))
				{
					features[f++] = &feat_touch;
				}
				else if(lilv_node_equals(feature, app->uris.ui_requestValue))
				{
					features[f++] = &feat_request_value;
				}
				else if(lilv_node_equals(feature, app->uris.ui_resize))
				{
					features[f++] = &feat_resize;
				}
				else if(lilv_node_equals(feature, app->uris.instance_access))
				{
					features[f++] = &feat_instance_access;
				}
				else if(lilv_node_equals(feature, app->uris.data_access))
				{
					features[f++] = &feat_data_access;
				}
				else if(lilv_node_equals(feature, app->uris.uri_map))
				{
					features[f++] = &feat_urimap;
				}
				else if(lilv_node_equals(feature, app->uris.opts_options))
				{
					features[f++] = &feat_opts;
				}
				else
				{
					//FIXME unknown feature
				}
			}

			lilv_nodes_free(required_features);
		}

		features[f] = NULL;
		assert(f <= MAX_FEATURES);
	}
#undef MAX_FEATURES

	{
		const LilvNode *uri = lilv_ui_get_uri(app->ui);
		unsigned n_opts = 0;

		LilvNodes *required_options = lilv_world_find_nodes(app->world, uri,
			app->uris.opts_requiredOption, NULL);
		if(required_options)
		{
			LILV_FOREACH(nodes, itr, required_options)
			{
				const LilvNode *option = lilv_nodes_get(required_options, itr);

				if(lilv_node_equals(option, app->uris.param_sampleRate))
				{
					opts[n_opts++] = opts_sampleRate;
				}
				else if(lilv_node_equals(option, app->uris.ui_updateRate))
				{
					opts[n_opts++] = opts_updateRate;
				}
				else
				{
					//FIXME unknown option
				}
			}

			lilv_nodes_free(required_options);
		}

		opts[n_opts++] = opts_sentinel; // sentinel
		assert(n_opts <= MAX_OPTS);
	}
#undef MAX_OPTS

	const LilvNode *ui_bundle_uri = lilv_ui_get_bundle_uri(app->ui);
	char *ui_plugin_bundle_path = lilv_file_uri_parse(lilv_node_as_string(ui_bundle_uri), NULL);
	if(ui_plugin_bundle_path)
	{
		if(app->ui_descriptor && app->ui_descriptor->instantiate)
		{
			app->ui_instance = app->ui_descriptor->instantiate(app->ui_descriptor,
				app->plugin_uri, ui_plugin_bundle_path, _write_function, app,
				(void *)&app->ui_widget, features);
		}

		lilv_free(ui_plugin_bundle_path);
	}

#if 0
	if(app->ui_instance && app->ui_idle_iface && app->ui_idle_iface->idle)
	{
		app->ui_idle_iface->idle(app->ui_instance);
	}
#endif

	for(unsigned i=0; i<tests_n; i++)
	{
		const test_t *test = &tests[i];
		res_t *res = &rets[i];

		res->is_whitelisted = lv2lint_test_is_whitelisted(app, app->ui_uri, test);
		res->urn = NULL;
		app->urn = &res->urn;
		res->ret = test->cb(app);
		const lint_t lnt = lv2lint_extract(app, res->ret);
		if(lnt & app->show)
		{
			msg = true;
		}
	}

	if(app->ui_instance)
	{
		if(app->ui_descriptor && app->ui_descriptor->cleanup)
		{
			app->ui_descriptor->cleanup(app->ui_instance);
		}

		app->ui_instance = NULL;
	}

	if(app->ui_widget)
	{
		app->ui_widget = 0;
	}

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

			lv2lint_report(app, test, res, show_passes, flag);
		}
	}

	for(unsigned i=0; i<tests_n; i++)
	{
		res_t *res = &rets[i];

		free(res->urn);
	}

jump:
	if(display)
	{
		if(win)
		{
			XDestroyWindow(display, win);
			win = 0;

			XFlush(display);
		}

		XCloseDisplay(display);
		display = NULL;
	}
}
