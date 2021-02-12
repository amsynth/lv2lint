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

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_xrm.h>

static void
_write_function(LV2UI_Controller controller, uint32_t index,
	uint32_t size, uint32_t protocol, const void *buf)
{
	app_t *app = controller;

	(void)app;
	(void)index;
	(void)size;
	(void)protocol;
	(void)buf;
	//FIXME
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

static const test_t tests [] = {
	{"UI Instantiation",    _test_ui_instantiation},
	{"UI Widget",           _test_ui_widget},
};

static const unsigned tests_n = sizeof(tests) / sizeof(test_t);

void
test_x11(app_t *app, bool *flag)
{
	bool msg = false;
	res_t *rets = alloca(tests_n * sizeof(res_t));
	if(!rets)
		return;

	xcb_connection_t *conn = NULL;
	xcb_screen_t *screen = NULL;
	xcb_drawable_t win = 0;

	conn = xcb_connect(NULL, NULL);
	if(!conn)
	{
		goto jump;
	}

	screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
	if(!screen)
	{
		goto jump;
	}

	win = xcb_generate_id(conn);
	if(!win)
	{
		goto jump;
	}

	const uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	const uint32_t values [2] = {
		screen->white_pixel,
		XCB_EVENT_MASK_STRUCTURE_NOTIFY
	};

	const xcb_void_cookie_t cookie = xcb_create_window_checked(conn,
		XCB_COPY_FROM_PARENT, win, screen->root, 0, 0, 640, 480, 0,
		XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);

	xcb_generic_error_t *error = xcb_request_check(conn, cookie);
	if(error)
	{
		free(error);
		goto jump;
	}

	xcb_map_window(conn, win);
	xcb_flush(conn);

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

	unsigned i = 0;
	//FIXME only add required ones
	const LV2_Feature *features [4];
	features[i++] = &feat_parent;
	features[i++] = &feat_map;
	features[i++] = &feat_unmap;
	features[i] = NULL;
	assert(i <= 4);

	app->ui_instance = app->descriptor->instantiate(app->descriptor,
		app->plugin_uri, "", _write_function, app, (void *)&app->ui_widget,
		features);

#if 0
	if(app->ui_instance && app->ui_idle_iface)
	{
		for(unsigned i=0; i<1000; i++)
		{
			app->ui_idle_iface->idle(app->ui_instance);
			usleep(1000);
		}
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
		app->descriptor->cleanup(app->ui_instance);
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
	if(conn)
	{
		if(win)
		{
			xcb_destroy_subwindows(conn, win);
			xcb_destroy_window(conn, win);
			win = 0;
		}

		xcb_disconnect(conn);
		conn = NULL;
	}
}
