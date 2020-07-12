/*
 * Copyright (c) 2016-2020 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#ifndef _LV2LINT_H
#define _LV2LINT_H

#include <unistd.h> // isatty
#include <string.h>
#include <stdlib.h>

#include <lilv/lilv.h>

#include <lv2/lv2plug.in/ns/ext/worker/worker.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>
#include <lv2/lv2plug.in/ns/ext/options/options.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>

#include <ardour.lv2/lv2_extensions.h>
#include <kx.lv2/lv2_external_ui.h>
#include <kx.lv2/lv2_programs.h>
#include <kx.lv2/lv2_rtmempool.h>

#ifdef ENABLE_ONLINE_TESTS
#	include <curl/curl.h>
#endif

#ifndef __unused
#	define __unused __attribute__((unused))
#endif

typedef enum _ansi_color_t {
	ANSI_COLOR_BOLD,
	ANSI_COLOR_RED,
	ANSI_COLOR_GREEN,
	ANSI_COLOR_YELLOW,
	ANSI_COLOR_BLUE,
	ANSI_COLOR_MAGENTA,
	ANSI_COLOR_CYAN,
	ANSI_COLOR_RESET,

	ANSI_COLOR_MAX
} ansi_color_t;

extern const char *colors [2][ANSI_COLOR_MAX];

typedef union _var_t var_t;
typedef struct _white_t white_t;
typedef struct _urid_t urid_t;
typedef struct _app_t app_t;
typedef struct _test_t test_t;
typedef struct _ret_t ret_t;
typedef struct _res_t res_t;
typedef const ret_t *(*test_cb_t)(app_t *app);

typedef enum _lint_t {
	LINT_NONE     = 0,
	LINT_NOTE     = (1 << 1),
	LINT_WARN     = (1 << 2),
	LINT_FAIL     = (1 << 3),
	LINT_PASS     = (1 << 4)
} lint_t;

struct _white_t {
	const char *uri;
	const char *pattern;
	white_t *next;
};

struct _urid_t {
	char *uri;
};

struct _ret_t {
	lint_t lnt;
	lint_t pck;
	const char *msg;
	const char *uri;
	const char *dsc;
};

struct _res_t {
	const ret_t *ret;
	char *urn;
	bool is_whitelisted;
};

union _var_t {
	uint32_t u32;
	int32_t i32;
	int64_t i64;
	float f32;
	double f64;
};

struct _app_t {
	LilvWorld *world;
	const char *plugin_uri;
	const LilvPlugin *plugin;
	LilvInstance *instance;
	const LV2UI_Descriptor *descriptor;
	const LilvPort *port;
	const LilvNode *parameter;
	const LilvUI *ui;
	const char *ui_uri;
	LilvNodes *writables;
	LilvNodes *readables;
	const LV2_Worker_Interface *work_iface;
	const LV2_Inline_Display_Interface *idisp_iface;
	const LV2_State_Interface *state_iface;
	const LV2_Options_Interface *opts_iface;
	const LV2UI_Idle_Interface *ui_idle_iface;
	const LV2UI_Show_Interface *ui_show_iface;
	const LV2UI_Resize *ui_resize_iface;
	var_t min;
	var_t max;
	var_t dflt;
	lint_t show;
	lint_t mask;
	bool pck;
	urid_t *urids;
	LV2_URID nurids;
	char **urn;
	unsigned n_include_dirs;
	char **include_dirs;
	white_t *whitelist_symbols;
	white_t *whitelist_libs;
	white_t *whitelist_tests;
	bool atty;
	bool debug;
	bool quiet;
#ifdef ENABLE_ONLINE_TESTS
	bool online;
	char *mail;
	bool mailto;
	CURL *curl;
	char *greet;
#endif
	struct {
		LilvNode *rdfs_label;
		LilvNode *rdfs_comment;
		LilvNode *rdfs_range;
		LilvNode *rdfs_subClassOf;

		LilvNode *rdf_type;
		LilvNode *rdf_value;

		LilvNode *doap_description;
		LilvNode *doap_license;
		LilvNode *doap_name;
		LilvNode *doap_shortdesc;

		LilvNode *lv2_minimum;
		LilvNode *lv2_maximum;
		LilvNode *lv2_Port;
		LilvNode *lv2_PortProperty;
		LilvNode *lv2_default;
		LilvNode *lv2_ControlPort;
		LilvNode *lv2_CVPort;
		LilvNode *lv2_AudioPort;
		LilvNode *lv2_OutputPort;
		LilvNode *lv2_InputPort;
		LilvNode *lv2_integer;
		LilvNode *lv2_toggled;
		LilvNode *lv2_Feature;
		LilvNode *lv2_minorVersion;
		LilvNode *lv2_microVersion;
		LilvNode *lv2_ExtensionData;
		LilvNode *lv2_requiredFeature;
		LilvNode *lv2_optionalFeature;
		LilvNode *lv2_extensionData;
		LilvNode *lv2_isLive;
		LilvNode *lv2_inPlaceBroken;
		LilvNode *lv2_hardRTCapable;
		LilvNode *lv2_documentation;
		LilvNode *lv2_sampleRate;
		LilvNode *lv2_InstrumentPlugin;
		LilvNode *lv2_scalePoint;

		LilvNode *atom_AtomPort;
		LilvNode *atom_Bool;
		LilvNode *atom_Int;
		LilvNode *atom_Long;
		LilvNode *atom_Float;
		LilvNode *atom_Double;
		LilvNode *atom_String;
		LilvNode *atom_Literal;
		LilvNode *atom_Path;
		LilvNode *atom_Chunk;
		LilvNode *atom_URI;
		LilvNode *atom_URID;
		LilvNode *atom_Tuple;
		LilvNode *atom_Object;
		LilvNode *atom_Vector;
		LilvNode *atom_Sequence;

		LilvNode *xsd_int;
		LilvNode *xsd_uint;
		LilvNode *xsd_long;
		LilvNode *xsd_float;
		LilvNode *xsd_double;

		LilvNode *state_loadDefaultState;
		LilvNode *state_state;
		LilvNode *state_interface;
		LilvNode *state_threadSafeRestore;
		LilvNode *state_makePath;
		LilvNode *state_freePath;

		LilvNode *work_schedule;
		LilvNode *work_interface;

		LilvNode *idisp_queue_draw;
		LilvNode *idisp_interface;

		LilvNode *opts_options;
		LilvNode *opts_interface;
		LilvNode *opts_requiredOption;
		LilvNode *opts_supportedOption;

		LilvNode *patch_writable;
		LilvNode *patch_readable;
		LilvNode *patch_Message;

		LilvNode *pg_group;

		LilvNode *ui_binary;
		LilvNode *ui_makeSONameResident;
		LilvNode *ui_idleInterface;
		LilvNode *ui_showInterface;
		LilvNode *ui_resize;
		LilvNode *ui_UI;
		LilvNode *ui_X11UI;
		LilvNode *ui_WindowsUI;
		LilvNode *ui_CocoaUI;
		LilvNode *ui_GtkUI;
		LilvNode *ui_Gtk3UI;
		LilvNode *ui_Qt4UI;
		LilvNode *ui_Qt5UI;

		LilvNode *event_EventPort;
		LilvNode *uri_map;
		LilvNode *instance_access;
		LilvNode *data_access;

		LilvNode *log_log;

		LilvNode *urid_map;
		LilvNode *urid_unmap;

		LilvNode *rsz_resize;

		LilvNode *bufsz_boundedBlockLength;
		LilvNode *bufsz_fixedBlockLength;
		LilvNode *bufsz_powerOf2BlockLength;
		LilvNode *bufsz_coarseBlockLength;

		LilvNode *pprops_supportsStrictBounds;

		LilvNode *param_sampleRate;

		LilvNode *bufsz_minBlockLength;
		LilvNode *bufsz_maxBlockLength;
		LilvNode *bufsz_nominalBlockLength;
		LilvNode *bufsz_sequenceSize;

		LilvNode *ui_updateRate;

		LilvNode *ext_Widget;

		LilvNode *morph_MorphPort;
		LilvNode *morph_AutoMorphPort;
		LilvNode *morph_supportsType;

		LilvNode *units_unit;
		LilvNode *units_Unit;
	} uris;
};

struct _test_t {
	const char *id;
	test_cb_t cb;
};

bool
test_plugin(app_t *app);

bool
test_port(app_t *app);

bool
test_parameter(app_t *app);

bool
test_ui(app_t *app);

#ifdef ENABLE_ONLINE_TESTS
bool
is_url(const char *uri);

bool
test_url(app_t *app, const char *url);
#endif

#ifdef ENABLE_ELF_TESTS
bool
test_visibility(app_t *app, const char *path, const char *uri,
	const char *description, char **symbols);

bool
check_for_symbol(app_t *app, const char *path, const char *description);

bool
test_shared_libraries(app_t *app, const char *path, const char *uri,
	const char *const *whitelist, unsigned n_whitelist,
	const char *const *blacklist, unsigned n_blacklist,
	char **libraries);
#endif

int
lv2lint_vprintf(app_t *app, const char *fmt, va_list args);

int
lv2lint_printf(app_t *app, const char *fmt, ...);

void
lv2lint_report(app_t *app, const test_t *test, res_t *res, bool show_passes, bool *flag);

lint_t
lv2lint_extract(app_t *app, const ret_t *ret);

bool
lv2lint_test_is_whitelisted(app_t *app, const char *uri, const test_t *test);

#endif
