/*
 * SPDX-FileCopyrightText: Hanspeter Portner <dev@open-music-kontrollers.ch>
 * SPDX-License-Identifier: Artistic-2.0
 */

#include <assert.h>

#include <lv2lint/lv2lint.h>

#include <lv2/log/log.h>
#include <lv2/atom/atom.h>
#include <lv2/parameters/parameters.h>
#include <lv2/instance-access/instance-access.h>
#include <lv2/data-access/data-access.h>

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
		.uri = LV2_UI__X11UI,
		.dsc = "You likely have forgotten to list all lv2:requiredFeatures."
	},
	ret_crash = {
		.lnt = LINT_FAIL,
		.msg = "crashed",
		.uri = LV2_CORE__Plugin,
		.dsc = "Well - fix your plugin."
	};

	const ret_t *ret = NULL;

	if(app->status.ui_instantiate)
	{
		ret = &ret_crash;
	}
	else if(!app->ui_instance)
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
		.uri = LV2_UI__X11UI,
		.dsc = "You likely have forgotten to return the proper XWindow ID."
	};

	const ret_t *ret = NULL;

	if(!app->ui_widget)
	{
		ret = &ret_widget;
	}

	return ret;
}

static Display *_display; //FIXME

static const ret_t *
_test_ui_hints(app_t *app)
{
	static const ret_t ret_fixed_aspect = {
		.lnt = LINT_WARN,
		.msg = "widget uses fixed aspect ratio",
		.uri = LV2_UI__X11UI,
		.dsc = "Windows with fixed aspect ratio are a pain in tiling window managers."
	},
	ret_aspect_constraints = {
		.lnt = LINT_WARN,
		.msg = "widget uses aspect ratio constraints",
		.uri = LV2_UI__X11UI,
		.dsc = "Windows with aspect ratio constraints are a pain in tiling window managers."
	},
	ret_fixed_size  = {
		.lnt = LINT_WARN,
		.msg = "widget uses fixed size",
		.uri = LV2_UI__X11UI,
		.dsc = "Windows with fixed sizes are a pain in tiling window managers."
	};

	const ret_t *ret = NULL;

	XSizeHints hints;
	memset(&hints, 0x1, sizeof(hints));
	long supplied = 0;

	if(_display && app->ui_widget)
	{
		XGetWMNormalHints(_display, app->ui_widget, &hints, &supplied);
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

static int
_wrap_instantiate(app_t *app, void *data)
{
	const LV2_Feature **features = data;

	if(!app->ui_descriptor)
	{
		return 1;
	}

	if(!app->ui_descriptor->instantiate)
	{
		return 1;
	}

	const LilvNode *ui_bundle_uri = lilv_ui_get_bundle_uri(app->ui);
	char *ui_plugin_bundle_path = lilv_file_uri_parse(lilv_node_as_string(ui_bundle_uri), NULL);

	if(!ui_plugin_bundle_path)
	{
		return 1;
	}

	char path [PATH_MAX];
	snprintf(path, sizeof(path), "%s", ui_plugin_bundle_path);

	lilv_free(ui_plugin_bundle_path);

	app->ui_instance = app->ui_descriptor->instantiate(app->ui_descriptor,
		app->plugin_uri, path, _write_function, app,
		(void *)&app->ui_widget, features);

	return 0;
}

static int
_wrap_cleanup(app_t *app, void *data __attribute__((unused)))
{
	if(!app->ui_instance)
	{
		return 1;
	}

	if(!app->ui_descriptor)
	{
		return 1;
	}

	if(!app->ui_descriptor->cleanup)
	{
		return 1;
	}

	app->ui_descriptor->cleanup(app->ui_instance);

	return 0;
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
	static bool xinit = true;

	if(xinit)
	{
		XInitThreads();
		xinit = false;
	}

	char *DISPLAY = getenv("DISPLAY");
	if(!DISPLAY || !strlen(DISPLAY))
	{
		return;
	}

	bool msg = false;
	res_t *rets = alloca(tests_n * sizeof(res_t));
	if(!rets)
	{
		return;
	}
	memset(rets, 0x0, tests_n * sizeof(res_t));

	Display *display = XOpenDisplay(NULL);
	if(!display)
	{
		goto jump;
	}
	_display = display;

	const int black = BlackPixel(display, DefaultScreen(display));
  Window win = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0,
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

	const float param_sample_rate = 48000.f;
	const float ui_update_rate = 25.f;

	const LV2_Options_Option opts_sampleRate = {
		.key = PARAMETERS__sampleRate,
		.size = sizeof(float),
		.type = ATOM__Float,
		.value = &param_sample_rate
	};
	const LV2_Options_Option opts_updateRate = {
		.key = UI__updateRate,
		.size = sizeof(float),
		.type = ATOM__Float,
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
		.data = app->instance ? lilv_instance_get_handle(app->instance) : NULL
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
			NODE(app, CORE__requiredFeature), NULL);
		if(required_features)
		{
			LILV_FOREACH(nodes, itr, required_features)
			{
				const LilvNode *feature = lilv_nodes_get(required_features, itr);
				const LV2_URID feat = app->map->map(app->map->handle, lilv_node_as_uri(feature));

				switch(feat)
				{
					case URID__map:
					{
						features[f++] = &feat_map;
					} break;
					case URID__unmap:
					{
						features[f++] = &feat_unmap;
					} break;
					case UI__parent:
					{
						features[f++] = &feat_parent;
					} break;
					case LOG__log:
					{
						features[f++] = &feat_log;
					} break;
					case UI__portMap:
					{
						features[f++] = &feat_port_map;
					} break;
					case UI__portSubscribe:
					{
						features[f++] = &feat_port_subscribe;
					} break;
					case UI__touch:
					{
						features[f++] = &feat_touch;
					} break;
					case UI__requestValue:
					{
						features[f++] = &feat_request_value;
					} break;
					case UI__resize:
					{
						features[f++] = &feat_resize;
					} break;
					case INSTANCE_ACCESS:
					{
						features[f++] = &feat_instance_access;
					} break;
					case DATA_ACCESS:
					{
						features[f++] = &feat_data_access;
					} break;
					case URI_MAP:
					{
						features[f++] = &feat_urimap;
					} break;
					case OPTIONS__options:
					{
						features[f++] = &feat_opts;
					} break;
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
			NODE(app, OPTIONS__requiredOption), NULL);
		if(required_options)
		{
			LILV_FOREACH(nodes, itr, required_options)
			{
				const LilvNode *option = lilv_nodes_get(required_options, itr);
				const LV2_URID opt = app->map->map(app->map->handle, lilv_node_as_uri(option));

				switch(opt)
				{
					case PARAMETERS__sampleRate:
					{
						opts[n_opts++] = opts_sampleRate;
					} break;
					case UI__updateRate:
					{
						opts[n_opts++] = opts_updateRate;
					} break;
				}
			}

			lilv_nodes_free(required_options);
		}

		opts[n_opts++] = opts_sentinel; // sentinel
		assert(n_opts <= MAX_OPTS);
	}
#undef MAX_OPTS

	// FIXME
#if 0
	app->status.ui_instantiate = lv2lint_wrap(app, _wrap_instantiate, (void *)features);
#else
	app->status.ui_instantiate = _wrap_instantiate(app, (void *)features);
#endif

	// FIXME
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

	// FIXME
#if 0
	app->status.ui_cleanup = lv2lint_wrap(app, _wrap_cleanup, NULL);
#else
	app->status.ui_cleanup = _wrap_cleanup(app, NULL);
#endif

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
		_display = NULL;
	}
}
