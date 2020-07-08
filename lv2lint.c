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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#if defined(HAS_FNMATCH)
#	include <fnmatch.h>
#endif

#include <lv2lint.h>

#include <lv2/lv2plug.in/ns/ext/patch/patch.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>
#include <lv2/lv2plug.in/ns/ext/log/log.h>
#include <lv2/lv2plug.in/ns/ext/port-groups/port-groups.h>
#include <lv2/lv2plug.in/ns/ext/uri-map/uri-map.h>
#include <lv2/lv2plug.in/ns/ext/event/event.h>
#include <lv2/lv2plug.in/ns/ext/morph/morph.h>
#include <lv2/lv2plug.in/ns/ext/uri-map/uri-map.h>
#include <lv2/lv2plug.in/ns/ext/instance-access/instance-access.h>
#include <lv2/lv2plug.in/ns/ext/parameters/parameters.h>
#include <lv2/lv2plug.in/ns/ext/port-props/port-props.h>
#include <lv2/lv2plug.in/ns/ext/buf-size/buf-size.h>
#include <lv2/lv2plug.in/ns/ext/resize-port/resize-port.h>
#include <lv2/lv2plug.in/ns/ext/options/options.h>
#include <lv2/lv2plug.in/ns/ext/data-access/data-access.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>
#include <lv2/lv2plug.in/ns/extensions/units/units.h>

#ifdef ENABLE_ELF_TESTS
#	include <fcntl.h>
#	include <libelf.h>
#	include <gelf.h>
#endif

#define MAPPER_API static inline
#define MAPPER_IMPLEMENTATION
#include <mapper.lv2/mapper.h>

const char *colors [2][ANSI_COLOR_MAX] = {
	{
		[ANSI_COLOR_BOLD]    = "",
		[ANSI_COLOR_RED]     = "",
		[ANSI_COLOR_GREEN]   = "",
		[ANSI_COLOR_YELLOW]  = "",
		[ANSI_COLOR_BLUE]    = "",
		[ANSI_COLOR_MAGENTA] = "",
		[ANSI_COLOR_CYAN]    = "",
		[ANSI_COLOR_RESET]   = ""
	},
	{
		[ANSI_COLOR_BOLD]    = "\x1b[1m",
		[ANSI_COLOR_RED]     = "\x1b[31m",
		[ANSI_COLOR_GREEN]   = "\x1b[32m",
		[ANSI_COLOR_YELLOW]  = "\x1b[33m",
		[ANSI_COLOR_BLUE]    = "\x1b[34m",
		[ANSI_COLOR_MAGENTA] = "\x1b[35m",
		[ANSI_COLOR_CYAN]    = "\x1b[36m",
		[ANSI_COLOR_RESET]   = "\x1b[0m"
	}
};

static void
_map_uris(app_t *app)
{
	app->uris.rdfs_label = lilv_new_uri(app->world, LILV_NS_RDFS"label");
	app->uris.rdfs_comment = lilv_new_uri(app->world, LILV_NS_RDFS"comment");
	app->uris.rdfs_range = lilv_new_uri(app->world, LILV_NS_RDFS"range");
	app->uris.rdfs_subClassOf = lilv_new_uri(app->world, LILV_NS_RDFS"subClassOf");

	app->uris.rdf_type = lilv_new_uri(app->world, LILV_NS_RDF"type");

	app->uris.doap_description = lilv_new_uri(app->world, LILV_NS_DOAP"description");
	app->uris.doap_license = lilv_new_uri(app->world, LILV_NS_DOAP"license");
	app->uris.doap_name = lilv_new_uri(app->world, LILV_NS_DOAP"name");
	app->uris.doap_shortdesc = lilv_new_uri(app->world, LILV_NS_DOAP"shortdesc");

	app->uris.lv2_minimum = lilv_new_uri(app->world, LV2_CORE__minimum);
	app->uris.lv2_maximum = lilv_new_uri(app->world, LV2_CORE__maximum);
	app->uris.lv2_Port = lilv_new_uri(app->world, LV2_CORE__Port);
	app->uris.lv2_PortProperty = lilv_new_uri(app->world, LV2_CORE__PortProperty);
	app->uris.lv2_default = lilv_new_uri(app->world, LV2_CORE__default);
	app->uris.lv2_ControlPort = lilv_new_uri(app->world, LV2_CORE__ControlPort);
	app->uris.lv2_CVPort = lilv_new_uri(app->world, LV2_CORE__CVPort);
	app->uris.lv2_AudioPort = lilv_new_uri(app->world, LV2_CORE__AudioPort);
	app->uris.lv2_OutputPort = lilv_new_uri(app->world, LV2_CORE__OutputPort);
	app->uris.lv2_InputPort = lilv_new_uri(app->world, LV2_CORE__InputPort);
	app->uris.lv2_integer = lilv_new_uri(app->world, LV2_CORE__integer);
	app->uris.lv2_toggled = lilv_new_uri(app->world, LV2_CORE__toggled);
	app->uris.lv2_Feature = lilv_new_uri(app->world, LV2_CORE__Feature);
	app->uris.lv2_minorVersion = lilv_new_uri(app->world, LV2_CORE__minorVersion);
	app->uris.lv2_microVersion = lilv_new_uri(app->world, LV2_CORE__microVersion);
	app->uris.lv2_ExtensionData = lilv_new_uri(app->world, LV2_CORE__ExtensionData);
	app->uris.lv2_requiredFeature = lilv_new_uri(app->world, LV2_CORE__requiredFeature);
	app->uris.lv2_optionalFeature = lilv_new_uri(app->world, LV2_CORE__optionalFeature);
	app->uris.lv2_extensionData = lilv_new_uri(app->world, LV2_CORE__extensionData);
	app->uris.lv2_isLive = lilv_new_uri(app->world, LV2_CORE__isLive);
	app->uris.lv2_inPlaceBroken = lilv_new_uri(app->world, LV2_CORE__inPlaceBroken);
	app->uris.lv2_hardRTCapable = lilv_new_uri(app->world, LV2_CORE__hardRTCapable);
	app->uris.lv2_documentation = lilv_new_uri(app->world, LV2_CORE__documentation);
	app->uris.lv2_sampleRate = lilv_new_uri(app->world, LV2_CORE__sampleRate);
	app->uris.lv2_InstrumentPlugin = lilv_new_uri(app->world, LV2_CORE__InstrumentPlugin);

	app->uris.atom_AtomPort = lilv_new_uri(app->world, LV2_ATOM__AtomPort);
	app->uris.atom_Bool = lilv_new_uri(app->world, LV2_ATOM__Bool);
	app->uris.atom_Int = lilv_new_uri(app->world, LV2_ATOM__Int);
	app->uris.atom_Long = lilv_new_uri(app->world, LV2_ATOM__Long);
	app->uris.atom_Float = lilv_new_uri(app->world, LV2_ATOM__Float);
	app->uris.atom_Double = lilv_new_uri(app->world, LV2_ATOM__Double);
	app->uris.atom_String = lilv_new_uri(app->world, LV2_ATOM__String);
	app->uris.atom_Literal = lilv_new_uri(app->world, LV2_ATOM__String);
	app->uris.atom_Path = lilv_new_uri(app->world, LV2_ATOM__Path);
	app->uris.atom_Chunk = lilv_new_uri(app->world, LV2_ATOM__Chunk);
	app->uris.atom_URI = lilv_new_uri(app->world, LV2_ATOM__URI);
	app->uris.atom_URID = lilv_new_uri(app->world, LV2_ATOM__URID);
	app->uris.atom_Tuple = lilv_new_uri(app->world, LV2_ATOM__Tuple);
	app->uris.atom_Object = lilv_new_uri(app->world, LV2_ATOM__Object);
	app->uris.atom_Vector = lilv_new_uri(app->world, LV2_ATOM__Vector);
	app->uris.atom_Sequence = lilv_new_uri(app->world, LV2_ATOM__Sequence);

	app->uris.xsd_int = lilv_new_uri(app->world, LILV_NS_XSD"int");
	app->uris.xsd_uint = lilv_new_uri(app->world, LILV_NS_XSD"nonNegativeInteger");
	app->uris.xsd_long = lilv_new_uri(app->world, LILV_NS_XSD"long");
	app->uris.xsd_float = lilv_new_uri(app->world, LILV_NS_XSD"float");
	app->uris.xsd_double = lilv_new_uri(app->world, LILV_NS_XSD"double");

	app->uris.state_loadDefaultState = lilv_new_uri(app->world, LV2_STATE__loadDefaultState);
	app->uris.state_state = lilv_new_uri(app->world, LV2_STATE__state);
	app->uris.state_interface = lilv_new_uri(app->world, LV2_STATE__interface);
	app->uris.state_threadSafeRestore = lilv_new_uri(app->world, LV2_STATE_PREFIX"threadSafeRestore");
	app->uris.state_makePath = lilv_new_uri(app->world, LV2_STATE__makePath);

	app->uris.work_schedule = lilv_new_uri(app->world, LV2_WORKER__schedule);
	app->uris.work_interface = lilv_new_uri(app->world, LV2_WORKER__interface);

	app->uris.idisp_queue_draw = lilv_new_uri(app->world, LV2_INLINEDISPLAY__queue_draw);
	app->uris.idisp_interface = lilv_new_uri(app->world, LV2_INLINEDISPLAY__interface);

	app->uris.opts_options = lilv_new_uri(app->world, LV2_OPTIONS__options);
	app->uris.opts_interface = lilv_new_uri(app->world, LV2_OPTIONS__interface);
	app->uris.opts_requiredOption = lilv_new_uri(app->world, LV2_OPTIONS__requiredOption);
	app->uris.opts_supportedOption = lilv_new_uri(app->world, LV2_OPTIONS__supportedOption);

	app->uris.patch_writable = lilv_new_uri(app->world, LV2_PATCH__writable);
	app->uris.patch_readable = lilv_new_uri(app->world, LV2_PATCH__readable);
	app->uris.patch_Message = lilv_new_uri(app->world, LV2_PATCH__Message);

	app->uris.pg_group = lilv_new_uri(app->world, LV2_PORT_GROUPS__group);

	app->uris.ui_binary = lilv_new_uri(app->world, LV2_UI__binary);
	app->uris.ui_makeSONameResident = lilv_new_uri(app->world, LV2_UI_PREFIX"makeSONameResident");
	app->uris.ui_idleInterface = lilv_new_uri(app->world, LV2_UI__idleInterface);
	app->uris.ui_showInterface = lilv_new_uri(app->world, LV2_UI__showInterface);
	app->uris.ui_resize = lilv_new_uri(app->world, LV2_UI__resize);
	app->uris.ui_UI= lilv_new_uri(app->world, LV2_UI__UI);
	app->uris.ui_X11UI = lilv_new_uri(app->world, LV2_UI__X11UI);
	app->uris.ui_WindowsUI = lilv_new_uri(app->world, LV2_UI__WindowsUI);
	app->uris.ui_CocoaUI = lilv_new_uri(app->world, LV2_UI__CocoaUI);
	app->uris.ui_GtkUI = lilv_new_uri(app->world, LV2_UI__GtkUI);
	app->uris.ui_Gtk3UI = lilv_new_uri(app->world, LV2_UI__Gtk3UI);
	app->uris.ui_Qt4UI = lilv_new_uri(app->world, LV2_UI__Qt4UI);
	app->uris.ui_Qt5UI = lilv_new_uri(app->world, LV2_UI__Qt5UI);

	app->uris.event_EventPort = lilv_new_uri(app->world, LV2_EVENT__EventPort);
	app->uris.uri_map = lilv_new_uri(app->world, LV2_URI_MAP_URI);
	app->uris.instance_access = lilv_new_uri(app->world, LV2_INSTANCE_ACCESS_URI);
	app->uris.data_access = lilv_new_uri(app->world, LV2_DATA_ACCESS_URI);

	app->uris.log_log = lilv_new_uri(app->world, LV2_LOG__log);

	app->uris.urid_map = lilv_new_uri(app->world, LV2_URID__map);
	app->uris.urid_unmap = lilv_new_uri(app->world, LV2_URID__unmap);

	app->uris.rsz_resize = lilv_new_uri(app->world, LV2_RESIZE_PORT__resize);

	app->uris.bufsz_boundedBlockLength = lilv_new_uri(app->world, LV2_BUF_SIZE__boundedBlockLength);
	app->uris.bufsz_fixedBlockLength = lilv_new_uri(app->world, LV2_BUF_SIZE__fixedBlockLength);
	app->uris.bufsz_powerOf2BlockLength = lilv_new_uri(app->world, LV2_BUF_SIZE__powerOf2BlockLength);
	app->uris.bufsz_coarseBlockLength = lilv_new_uri(app->world, LV2_BUF_SIZE_PREFIX"coarseBlockLength");

	app->uris.pprops_supportsStrictBounds = lilv_new_uri(app->world, LV2_PORT_PROPS__supportsStrictBounds);

	app->uris.param_sampleRate = lilv_new_uri(app->world, LV2_PARAMETERS__sampleRate);

	app->uris.bufsz_minBlockLength = lilv_new_uri(app->world, LV2_BUF_SIZE__minBlockLength);
	app->uris.bufsz_maxBlockLength = lilv_new_uri(app->world, LV2_BUF_SIZE__maxBlockLength);
	app->uris.bufsz_nominalBlockLength = lilv_new_uri(app->world, LV2_BUF_SIZE__nominalBlockLength);
	app->uris.bufsz_sequenceSize = lilv_new_uri(app->world, LV2_BUF_SIZE__sequenceSize);

	app->uris.ui_updateRate = lilv_new_uri(app->world, LV2_UI__updateRate);

	app->uris.ext_Widget = lilv_new_uri(app->world, LV2_EXTERNAL_UI__Widget);

	app->uris.morph_MorphPort = lilv_new_uri(app->world, LV2_MORPH__MorphPort);
	app->uris.morph_AutoMorphPort = lilv_new_uri(app->world, LV2_MORPH__AutoMorphPort);
	app->uris.morph_supportsType = lilv_new_uri(app->world, LV2_MORPH__supportsType);

	app->uris.units_unit = lilv_new_uri(app->world, LV2_UNITS__unit);
	app->uris.units_Unit = lilv_new_uri(app->world, LV2_UNITS__Unit);
}

static void
_unmap_uris(app_t *app)
{
	lilv_node_free(app->uris.rdfs_label);
	lilv_node_free(app->uris.rdfs_comment);
	lilv_node_free(app->uris.rdfs_range);
	lilv_node_free(app->uris.rdfs_subClassOf);

	lilv_node_free(app->uris.rdf_type);

	lilv_node_free(app->uris.doap_description);
	lilv_node_free(app->uris.doap_license);
	lilv_node_free(app->uris.doap_name);
	lilv_node_free(app->uris.doap_shortdesc);

	lilv_node_free(app->uris.lv2_minimum);
	lilv_node_free(app->uris.lv2_maximum);
	lilv_node_free(app->uris.lv2_Port);
	lilv_node_free(app->uris.lv2_PortProperty);
	lilv_node_free(app->uris.lv2_default);
	lilv_node_free(app->uris.lv2_ControlPort);
	lilv_node_free(app->uris.lv2_CVPort);
	lilv_node_free(app->uris.lv2_AudioPort);
	lilv_node_free(app->uris.lv2_OutputPort);
	lilv_node_free(app->uris.lv2_InputPort);
	lilv_node_free(app->uris.lv2_integer);
	lilv_node_free(app->uris.lv2_toggled);
	lilv_node_free(app->uris.lv2_Feature);
	lilv_node_free(app->uris.lv2_minorVersion);
	lilv_node_free(app->uris.lv2_microVersion);
	lilv_node_free(app->uris.lv2_ExtensionData);
	lilv_node_free(app->uris.lv2_requiredFeature);
	lilv_node_free(app->uris.lv2_optionalFeature);
	lilv_node_free(app->uris.lv2_extensionData);
	lilv_node_free(app->uris.lv2_isLive);
	lilv_node_free(app->uris.lv2_inPlaceBroken);
	lilv_node_free(app->uris.lv2_hardRTCapable);
	lilv_node_free(app->uris.lv2_documentation);
	lilv_node_free(app->uris.lv2_sampleRate);
	lilv_node_free(app->uris.lv2_InstrumentPlugin);

	lilv_node_free(app->uris.atom_AtomPort);
	lilv_node_free(app->uris.atom_Bool);
	lilv_node_free(app->uris.atom_Int);
	lilv_node_free(app->uris.atom_Long);
	lilv_node_free(app->uris.atom_Float);
	lilv_node_free(app->uris.atom_Double);
	lilv_node_free(app->uris.atom_String);
	lilv_node_free(app->uris.atom_Literal);
	lilv_node_free(app->uris.atom_Path);
	lilv_node_free(app->uris.atom_Chunk);
	lilv_node_free(app->uris.atom_URI);
	lilv_node_free(app->uris.atom_URID);
	lilv_node_free(app->uris.atom_Tuple);
	lilv_node_free(app->uris.atom_Object);
	lilv_node_free(app->uris.atom_Vector);
	lilv_node_free(app->uris.atom_Sequence);

	lilv_node_free(app->uris.state_loadDefaultState);
	lilv_node_free(app->uris.state_state);
	lilv_node_free(app->uris.state_interface);
	lilv_node_free(app->uris.state_threadSafeRestore);
	lilv_node_free(app->uris.state_makePath);

	lilv_node_free(app->uris.work_schedule);
	lilv_node_free(app->uris.work_interface);

	lilv_node_free(app->uris.idisp_queue_draw);
	lilv_node_free(app->uris.idisp_interface);

	lilv_node_free(app->uris.opts_options);
	lilv_node_free(app->uris.opts_interface);
	lilv_node_free(app->uris.opts_requiredOption);
	lilv_node_free(app->uris.opts_supportedOption);

	lilv_node_free(app->uris.patch_writable);
	lilv_node_free(app->uris.patch_readable);
	lilv_node_free(app->uris.patch_Message);

	lilv_node_free(app->uris.pg_group);

	lilv_node_free(app->uris.ui_binary);
	lilv_node_free(app->uris.ui_makeSONameResident);
	lilv_node_free(app->uris.ui_idleInterface);
	lilv_node_free(app->uris.ui_showInterface);
	lilv_node_free(app->uris.ui_resize);
	lilv_node_free(app->uris.ui_UI);
	lilv_node_free(app->uris.ui_X11UI);
	lilv_node_free(app->uris.ui_WindowsUI);
	lilv_node_free(app->uris.ui_CocoaUI);
	lilv_node_free(app->uris.ui_GtkUI);
	lilv_node_free(app->uris.ui_Gtk3UI);
	lilv_node_free(app->uris.ui_Qt4UI);
	lilv_node_free(app->uris.ui_Qt5UI);

	lilv_node_free(app->uris.event_EventPort);
	lilv_node_free(app->uris.uri_map);
	lilv_node_free(app->uris.instance_access);
	lilv_node_free(app->uris.data_access);

	lilv_node_free(app->uris.log_log);

	lilv_node_free(app->uris.urid_map);
	lilv_node_free(app->uris.urid_unmap);

	lilv_node_free(app->uris.rsz_resize);

	lilv_node_free(app->uris.bufsz_boundedBlockLength);
	lilv_node_free(app->uris.bufsz_fixedBlockLength);
	lilv_node_free(app->uris.bufsz_powerOf2BlockLength);
	lilv_node_free(app->uris.bufsz_coarseBlockLength);

	lilv_node_free(app->uris.pprops_supportsStrictBounds);

	lilv_node_free(app->uris.param_sampleRate);

	lilv_node_free(app->uris.bufsz_minBlockLength);
	lilv_node_free(app->uris.bufsz_maxBlockLength);
	lilv_node_free(app->uris.bufsz_nominalBlockLength);
	lilv_node_free(app->uris.bufsz_sequenceSize);

	lilv_node_free(app->uris.ui_updateRate);

	lilv_node_free(app->uris.ext_Widget);

	lilv_node_free(app->uris.morph_MorphPort);
	lilv_node_free(app->uris.morph_AutoMorphPort);
	lilv_node_free(app->uris.morph_supportsType);

	lilv_node_free(app->uris.units_unit);
	lilv_node_free(app->uris.units_Unit);
}

static void
_free_urids(app_t *app)
{
	for(unsigned i = 0; i < app->nurids; i++)
	{
		urid_t *itm = &app->urids[i];

		if(itm->uri)
			free(itm->uri);
	}
	free(app->urids);

	app->urids = NULL;
	app->nurids = 0;
}

static LV2_Worker_Status
_respond(LV2_Worker_Respond_Handle instance, uint32_t size, const void *data)
{
	app_t *app = instance;

	if(app->work_iface && app->work_iface->work_response)
		return app->work_iface->work_response(&app->instance, size, data);

	else return LV2_WORKER_ERR_UNKNOWN;
}

static LV2_Worker_Status
_sched(LV2_Worker_Schedule_Handle instance, uint32_t size, const void *data)
{
	app_t *app = instance;

	LV2_Worker_Status status = LV2_WORKER_SUCCESS;
	if(app->work_iface && app->work_iface->work)
		status |= app->work_iface->work(&app->instance, _respond, app, size, data);
	if(app->work_iface && app->work_iface->end_run)
		status |= app->work_iface->end_run(&app->instance);

	return status;
}

#if defined(_WIN32)
static inline char *
strsep(char **sp, const char *sep)
{
	char *p, *s;
	if(sp == NULL || *sp == NULL || **sp == '\0')
		return(NULL);
	s = *sp;
	p = s + strcspn(s, sep);
	if(*p != '\0')
		*p++ = '\0';
	*sp = p;
	return(s);
}
#endif

static int
_vprintf(void *data __unused, LV2_URID type __unused, const char *fmt,
	va_list args)
{
	char *buf = NULL;

	if(asprintf(&buf, fmt, args) == -1)
	{
		buf = NULL;
	}

	if(buf)
	{
		const char *sep = "\n\0";
		for(char *bufp = buf, *ptr = strsep(&bufp, sep);
			ptr;
			ptr = strsep(&bufp, sep) )
		{
			fprintf(stderr, "%s\n", ptr);
		}

		free(buf);
	}

	return 0;
}

static int
_printf(void *data, LV2_URID type, const char *fmt, ...)
{
  va_list args;
	int ret;

  va_start (args, fmt);
	ret = _vprintf(data, type, fmt, args);
  va_end(args);

	return ret;
}

static char *
_mkpath(LV2_State_Make_Path_Handle instance __unused, const char *abstract_path)
{
	char *absolute_path = NULL;

	if(asprintf(&absolute_path, "/tmp/%s", abstract_path) == -1)
		absolute_path = NULL;

	return absolute_path;
}

static LV2_Resize_Port_Status
_resize(LV2_Resize_Port_Feature_Data instance __unused, uint32_t index __unused,
	size_t size __unused)
{
	return LV2_RESIZE_PORT_SUCCESS;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
static uint32_t
_uri_to_id(LV2_URI_Map_Callback_Data instance,
	const char *_map __attribute__((unused)), const char *uri)
{
	LV2_URID_Map *map = instance;

	return map->map(map->handle, uri);
}
#pragma GCC diagnostic pop

static void
_queue_draw(LV2_Inline_Display_Handle instance)
{
	app_t *app = instance;
	(void)app;
}

static void
_header(char **argv)
{
	fprintf(stderr,
		"%s "LV2LINT_VERSION"\n"
		"Copyright (c) 2016-2020 Hanspeter Portner (dev@open-music-kontrollers.ch)\n"
		"Released under Artistic License 2.0 by Open Music Kontrollers\n",
		argv[0]);
}

static void
_version(char **argv)
{
	_header(argv);

	fprintf(stderr,
		"--------------------------------------------------------------------\n"
		"This is free software: you can redistribute it and/or modify\n"
		"it under the terms of the Artistic License 2.0 as published by\n"
		"The Perl Foundation.\n"
		"\n"
		"This source is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
		"Artistic License 2.0 for more details.\n"
		"\n"
		"You should have received a copy of the Artistic License 2.0\n"
		"along the source as a COPYING file. If not, obtain it from\n"
		"http://www.perlfoundation.org/artistic_license_2_0.\n\n");
}

static void
_usage(char **argv)
{
	_header(argv);

	fprintf(stderr,
		"--------------------------------------------------------------------\n"
		"USAGE\n"
		"   %s [OPTIONS] {PLUGIN_URI}*\n"
		"\n"
		"OPTIONS\n"
		"   [-v]                         print version information\n"
		"   [-h]                         print usage information\n"
		"   [-q]                         quiet mode, show only a summary\n"
		"   [-d]                         show verbose test item documentation\n"
		"   [-I] INCLUDE_DIR             use include directory to search for plugins"
		                                 " (can be used multiple times)\n"
		"   [-u] URI_PATTERN             URI pattern (shell wildcards) for whitelist patterns\n"
		"   [-t] TEST_PATTERN            test name pattern (shell wildcards) to whitelist\n"
#ifdef ENABLE_ELF_TESTS
		"   [-s] SYMBOL_PATTERN          symbol pattern (shell wildcards) to whitelist"
		                                 " (can be used multiple times)\n"
		"   [-l] LIBRARY_PATTERN         library pattern (shell wildcards) to whitelist"
		                                 " (can be used multiple times)\n"
#endif
#ifdef ENABLE_ONLINE_TESTS
		"   [-o]                         run online test items\n"
		"   [-m]                         create mail to plugin author\n"
		"   [-g] GREETER                 custom mail greeter\n"
#endif

		"   [-M] (no)pack                skip some tests for distribution packagers\n"
		"   [-S] (no)warn|note|pass|all  show warnings, notes, passes or all\n"
		"   [-E] (no)warn|note|all       treat warnings, notes or all as errors\n\n"
		, argv[0]);
}

#ifdef ENABLE_ONLINE_TESTS
static const char *http_prefix = "http://";
static const char *https_prefix = "https://";
static const char *ftp_prefix = "ftp://";
static const char *ftps_prefix = "ftps://";

bool
is_url(const char *uri)
{
	const bool is_http = strncmp(uri, http_prefix, strlen(http_prefix));
	const bool is_https = strncmp(uri, https_prefix, strlen(https_prefix));
	const bool is_ftp = strncmp(uri, ftp_prefix, strlen(ftp_prefix));
	const bool is_ftps = strncmp(uri, ftps_prefix, strlen(ftps_prefix));

	return is_http || is_https || is_ftp || is_ftps;
}

bool
test_url(app_t *app, const char *url)
{
	curl_easy_setopt(app->curl, CURLOPT_URL, url);
	curl_easy_setopt(app->curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(app->curl, CURLOPT_NOBODY, 1);
	curl_easy_setopt(app->curl, CURLOPT_CONNECTTIMEOUT, 10L); // secs
	curl_easy_setopt(app->curl, CURLOPT_TIMEOUT, 20L); //secs

	const CURLcode resp = curl_easy_perform(app->curl);

	long http_code;
	curl_easy_getinfo(app->curl, CURLINFO_RESPONSE_CODE, &http_code);

	if( (resp == CURLE_OK) && (http_code == 200) )
	{
		return true;
	}

	return false;
}
#endif

#ifdef ENABLE_ELF_TESTS
static void
_append_to(char **dst, const char *src)
{
	static const char *prefix = "\n                * ";

	if(*dst)
	{
		const size_t sz = strlen(*dst) + strlen(prefix) + strlen(src) + 1;
		*dst = realloc(*dst, sz);
		strcat(*dst, prefix);
		strcat(*dst, src);
	}
	else
	{
		const size_t sz = strlen(src) + strlen(prefix) + 1;
		*dst = malloc(sz);
		strcpy(*dst, prefix);
		strcat(*dst, src);
	}
}

bool
test_visibility(app_t *app, const char *path, const char *description, char **symbols)
{
	static const char *whitelist [] = {
		// LV2
		"lv2_descriptor",
		"lv2ui_descriptor",
		"lv2_dyn_manifest_open",
		"lv2_dyn_manifest_get_subjects",
		"lv2_dyn_manifest_get_data",
		"lv2_dyn_manifest_close",
		// C
		"_init",
		"_fini",
		"_edata",
		"_end",
		"__bss_start",
		// Rust
		"__rdl_alloc",
		"__rdl_alloc_excess",
		"__rdl_alloc_zeroed",
		"__rdl_dealloc",
		"__rdl_grow_in_place",
		"__rdl_oom",
		"__rdl_realloc",
		"__rdl_realloc_excess",
		"__rdl_shrink_in_place",
		"__rdl_usable_size",
		"rust_eh_personality"
	};
	const unsigned n_whitelist = sizeof(whitelist) / sizeof(const char *);
	bool desc = false;
	unsigned invalid = 0;

	const int fd = open(path, O_RDONLY);
	if(fd != -1)
	{
		elf_version(EV_CURRENT);

		Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
		if(elf)
		{
			for(Elf_Scn *scn = elf_nextscn(elf, NULL);
				scn;
				scn = elf_nextscn(elf, scn))
			{
				GElf_Shdr shdr;
				memset(&shdr, 0x0, sizeof(GElf_Shdr));
				gelf_getshdr(scn, &shdr);

				if( (shdr.sh_type == SHT_SYMTAB) || (shdr.sh_type == SHT_DYNSYM) )
				{
					// found a symbol table
					Elf_Data *data = elf_getdata(scn, NULL);
					const unsigned count = shdr.sh_size / shdr.sh_entsize;

					// iterate over symbol names
					for(unsigned i = 0; i < count; i++)
					{
						GElf_Sym sym;
						memset(&sym, 0x0, sizeof(GElf_Sym));
						gelf_getsym(data, i, &sym);

						const bool is_global = GELF_ST_BIND(sym.st_info) == STB_GLOBAL;
						if(sym.st_value && is_global)
						{
							const char *name = elf_strptr(elf, shdr.sh_link, sym.st_name);

							if(!strcmp(name, description))
							{
								desc = true;
							}
							else
							{
								bool whitelist_match = false;

								for(unsigned j = 0; j < n_whitelist; j++)
								{
									if(!strcmp(name, whitelist[j]))
									{
										whitelist_match = true;
										break;
									}
								}

								if(!whitelist_match)
								{
									for(unsigned j = 0; j < app->n_whitelist_symbols; j++)
									{
										char *whitelist_symbol = app->whitelist_symbols
											? app->whitelist_symbols[j]
											: NULL;

										if(!whitelist_symbol)
										{
											continue;
										}

#if defined(HAS_FNMATCH)
										if(fnmatch(whitelist_symbol, name,
											FNM_CASEFOLD | FNM_EXTMATCH) == 0)
#else
										if(strcasecmp(whitelist_symbol, name) == 0)
#endif
										{
											whitelist_match = true;
											break;
										}
									}
								}

								if(!whitelist_match)
								{
									if(invalid <= 10)
									{
										_append_to(symbols, (invalid == 10)
											? "... there is more, but the rest is being truncated"
											: name);
									}
									invalid++;
								}
							}
						}
					}

					break;
				}
			}
			elf_end(elf);
		}
		close(fd);
	}

	return !(!desc || invalid);
}

bool
test_shared_libraries(app_t *app, const char *path, const char *const *whitelist,
	unsigned n_whitelist, const char *const *blacklist, unsigned n_blacklist,
	char **libraries)
{
	unsigned invalid = 0;

	const int fd = open(path, O_RDONLY);
	if(fd != -1)
	{
		elf_version(EV_CURRENT);

		Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
		if(elf)
		{
			for(Elf_Scn *scn = elf_nextscn(elf, NULL);
				scn;
				scn = elf_nextscn(elf, scn))
			{
				GElf_Shdr shdr;
				memset(&shdr, 0x0, sizeof(GElf_Shdr));
				gelf_getshdr(scn, &shdr);

				if(shdr.sh_type == SHT_DYNAMIC)
				{
					// found a dynamic table
					Elf_Data *data = elf_getdata(scn, NULL);
					const unsigned count = shdr.sh_size / shdr.sh_entsize;

					// iterate over linked shared library names
					for(unsigned i = 0; i < count; i++)
					{
						GElf_Dyn dyn;
						memset(&dyn, 0x0, sizeof(GElf_Dyn));
						gelf_getdyn(data, i, &dyn);

						if(dyn.d_tag == DT_NEEDED)
						{
							const char *name = elf_strptr(elf, shdr.sh_link, dyn.d_un.d_val);

							bool whitelist_match = false;
							bool blacklist_match = false;

							for(unsigned j = 0; j < n_whitelist; j++)
							{
								if(!strncmp(name, whitelist[j], strlen(whitelist[j])))
								{
									whitelist_match = true;
									break;
								}
							}

							for(unsigned j = 0; j < app->n_whitelist_libs; j++)
							{
								char *whitelist_lib = app->whitelist_libs
									? app->whitelist_libs[j]
									: NULL;

								if(!whitelist_lib)
								{
									continue;
								}

#if defined(HAS_FNMATCH)
								if(fnmatch(whitelist_lib, name,
									FNM_CASEFOLD | FNM_EXTMATCH) == 0)
#else
								if(strcasecmp(whitelist_lib, name) == 0)
#endif
								{
									whitelist_match = true;
									break;
								}
							}

							for(unsigned j = 0; j < n_blacklist; j++)
							{
								if(!strncmp(name, blacklist[j], strlen(blacklist[j])))
								{
									blacklist_match = true;
									break;
								}
							}

							if(n_whitelist && !whitelist_match)
							{
								_append_to(libraries, name);
								invalid++;
							}
							if(n_blacklist && blacklist_match && !whitelist_match)
							{
								_append_to(libraries, name);
								invalid++;
							}
						}
						//FIXME
					}

					break;
				}
			}
			elf_end(elf);
		}
		close(fd);
	}

	return !invalid;
}
#endif

static void
_state_set_value(const char *symbol __unused, void *data __unused,
	const void *value __unused, uint32_t size __unused, uint32_t type __unused)
{
	//FIXME
}

static white_t *
_white_append(white_t *parent, const char *uri, const char *pattern)
{
	white_t *white = calloc(1, sizeof(white_t));

	if(!white)
	{
		return parent;
	}

	white->uri = uri;
	white->pattern = pattern;
	white->next = parent;

	return white;
}

static white_t *
_white_remove(white_t *parent)
{
	white_t *next = NULL;

	if(parent)
	{
		next = parent->next;
		free(parent);
	}

	return next;
}

static white_t *
_white_free(white_t *white)
{
	while(white)
	{
		white = _white_remove(white);
	}

	return NULL;
}

static bool
_pattern_match(const char *pattern, const char *str)
{
	if(pattern == NULL)
	{
		return true;
	}

#if defined(HAS_FNMATCH)
	if(fnmatch(pattern, str, FNM_CASEFOLD | FNM_EXTMATCH) == 0)
#else
	if(strcasecmp(pattern, str) == 0)
#endif
	{
		return true;
	}

	return false;
}

static bool
_white_match(white_t *white, const char *uri, const char *str)
{
	for( ; white; white = white->next)
	{
		if(_pattern_match(white->uri, uri) && _pattern_match(white->pattern, str))
		{
			return true;
		}
	}

	return false;
}

static void
_append_include_dir(app_t *app, char *include_dir)
{
	char **include_dirs = realloc(app->include_dirs,
		(app->n_include_dirs + 1) * sizeof(const char *));
	if(!include_dirs)
	{
		return;
	}

	app->include_dirs = include_dirs;

	size_t len = strlen(include_dir) + 1;

	if(include_dir[len - 2] == '/')
	{
		char *dst = malloc(len);

		if(dst)
		{
			app->include_dirs[app->n_include_dirs] = dst;
			snprintf(app->include_dirs[app->n_include_dirs], len, "%s", include_dir);

			app->n_include_dirs++;
		}
	}
	else
	{
		len++;
		char *dst = malloc(len);

		if(dst)
		{
			app->include_dirs[app->n_include_dirs] = dst;
			snprintf(app->include_dirs[app->n_include_dirs], len, "%s/", include_dir);

			app->n_include_dirs++;
		}
	}
}

static void
_load_include_dirs(app_t *app)
{
	for(unsigned i = 0; i < app->n_include_dirs; i++)
	{
		char *include_dir = app->include_dirs ? app->include_dirs[i] : NULL;

		if(!include_dir)
		{
			continue;
		}

		LilvNode *bundle_node = lilv_new_file_uri(app->world, NULL, include_dir);

		if(bundle_node)
		{
			lilv_world_load_bundle(app->world, bundle_node);
			lilv_world_load_resource(app->world, bundle_node);

			lilv_node_free(bundle_node);
		}
	}
}

static void
_free_include_dirs(app_t *app)
{
	for(unsigned i = 0; i < app->n_include_dirs; i++)
	{
		char *include_dir = app->include_dirs ? app->include_dirs[i] : NULL;

		if(!include_dir)
		{
			continue;
		}

		LilvNode *bundle_node = lilv_new_file_uri(app->world, NULL, include_dir);

		if(bundle_node)
		{
			lilv_world_unload_resource(app->world, bundle_node);
			lilv_world_unload_bundle(app->world, bundle_node);

			lilv_node_free(bundle_node);
		}

		free(include_dir);
	}

	if(app->include_dirs)
	{
		free(app->include_dirs);
	}
}

static void
_append_whitelist_test(app_t *app, const char *uri, const char *pattern)
{
	app->whitelist_tests = _white_append(app->whitelist_tests, uri, pattern);
}

static void
_free_whitelist_tests(app_t *app)
{
	app->whitelist_tests = _white_free(app->whitelist_tests);
}

bool
lv2lint_test_is_whitelisted(app_t *app, const char *uri, const test_t *test)
{
	return _white_match(app->whitelist_tests, uri, test->id);
}

#ifdef ENABLE_ELF_TESTS
static void
_append_whitelist_symbol(app_t *app, char *whitelist_symbol)
{
	char **whitelist_symbols = realloc(app->whitelist_symbols,
		(app->n_whitelist_symbols + 1) * sizeof(const char *));
	if(!whitelist_symbols)
	{
		return;
	}

	app->whitelist_symbols = whitelist_symbols;

	size_t len = strlen(whitelist_symbol) + 1;

	char *dst = malloc(len);

	if(dst)
	{
		app->whitelist_symbols[app->n_whitelist_symbols] = dst;
		snprintf(app->whitelist_symbols[app->n_whitelist_symbols], len, "%s", whitelist_symbol);

		app->n_whitelist_symbols++;
	}
}

static void
_free_whitelist_symbols(app_t *app)
{
	for(unsigned i = 0; i < app->n_whitelist_symbols; i++)
	{
		char *whitelist_symbol = app->whitelist_symbols ? app->whitelist_symbols[i] : NULL;

		if(!whitelist_symbol)
		{
			continue;
		}

		free(whitelist_symbol);
	}

	if(app->whitelist_symbols)
	{
		free(app->whitelist_symbols);
	}
}

static void
_append_whitelist_lib(app_t *app, char *whitelist_lib)
{
	char **whitelist_libs = realloc(app->whitelist_libs,
		(app->n_whitelist_libs + 1) * sizeof(const char *));
	if(!whitelist_libs)
	{
		return;
	}

	app->whitelist_libs = whitelist_libs;

	size_t len = strlen(whitelist_lib) + 1;

	char *dst = malloc(len);

	if(dst)
	{
		app->whitelist_libs[app->n_whitelist_libs] = dst;
		snprintf(app->whitelist_libs[app->n_whitelist_libs], len, "%s", whitelist_lib);

		app->n_whitelist_libs++;
	}
}

static void
_free_whitelist_libs(app_t *app)
{
	for(unsigned i = 0; i < app->n_whitelist_libs; i++)
	{
		char *whitelist_lib = app->whitelist_libs ? app->whitelist_libs[i] : NULL;

		if(!whitelist_lib)
		{
			continue;
		}

		free(whitelist_lib);
	}

	if(app->whitelist_libs)
	{
		free(app->whitelist_libs);
	}
}
#endif

int
main(int argc, char **argv)
{
	static app_t app;
	const char *uri = NULL;
	app.atty = isatty(1);
	app.show = LINT_FAIL | LINT_WARN; // always report failed and warned tests
	app.mask = LINT_FAIL; // always fail at failed tests
	app.pck = true;
#ifdef ENABLE_ONLINE_TESTS
	app.greet = "Dear LV2 plugin developer\n"
		"\n"
		"We would like to congratulate you for your efforts to have created this\n"
		"awesome plugin for the LV2 ecosystem.\n"
		"\n"
		"However, we have found some minor issues where your plugin deviates from\n"
		"the LV2 plugin specification and/or its best implementation practices.\n"
		"By fixing those, you can make your plugin more conforming and thus likely\n"
		"usable in more hosts and with less issues for your users.\n"
		"\n"
		"Kindly find below an automatically generated bug report with a summary\n"
		"of potential issues.\n"
		"\n"
		"Yours sincerely\n"
		"                                 /The unofficial LV2 inquisitorial squad/\n"
		"\n"
		"---\n\n";
#endif

	int c;
	while( (c = getopt(argc, argv, "vhqdM:S:E:I:u:t:"
#ifdef ENABLE_ONLINE_TESTS
		"omg:"
#endif
#ifdef ENABLE_ELF_TESTS
		"s:l:"
#endif
		) ) != -1)
	{
		switch(c)
		{
			case 'v':
				_version(argv);
				return 0;
			case 'h':
				_usage(argv);
				return 0;
			case 'q':
				app.quiet = true;
				break;
			case 'd':
				app.debug = true;
				break;
			case 'I':
				_append_include_dir(&app, optarg);
				break;
			case 'u':
				uri = optarg;
				break;
			case 't':
				_append_whitelist_test(&app, uri, optarg);
				break;
#ifdef ENABLE_ELF_TESTS
			case 's':
				_append_whitelist_symbol(&app, optarg);
				break;
			case 'l':
				_append_whitelist_lib(&app, optarg);
				break;
#endif
#ifdef ENABLE_ONLINE_TESTS
			case 'o':
				app.online = true;
				break;
			case 'm':
				app.mailto = true;
				app.atty = false;
				break;
			case 'g':
				app.greet = optarg;
				break;
#endif
			case 'M':
				if(!strcmp(optarg, "pack"))
				{
					app.pck = true;
				}

				else if(!strcmp(optarg, "nopack"))
				{
					app.pck = false;
				}

				break;
			case 'S':
				if(!strcmp(optarg, "warn"))
				{
					app.show |= LINT_WARN;
				}
				else if(!strcmp(optarg, "note"))
				{
					app.show |= LINT_NOTE;
				}
				else if(!strcmp(optarg, "pass"))
				{
					app.show |= LINT_PASS;
				}
				else if(!strcmp(optarg, "all"))
				{
					app.show |= (LINT_WARN | LINT_NOTE | LINT_PASS);
				}

				else if(!strcmp(optarg, "nowarn"))
				{
					app.show &= ~LINT_WARN;
				}
				else if(!strcmp(optarg, "nonote"))
				{
					app.show &= ~LINT_NOTE;
				}
				else if(!strcmp(optarg, "nopass"))
				{
					app.show &= ~LINT_PASS;
				}
				else if(!strcmp(optarg, "noall"))
				{
					app.show &= ~(LINT_WARN | LINT_NOTE | LINT_PASS);
				}

				break;
			case 'E':
				if(!strcmp(optarg, "warn"))
				{
					app.show |= LINT_WARN;
					app.mask |= LINT_WARN;
				}
				else if(!strcmp(optarg, "note"))
				{
					app.show |= LINT_NOTE;
					app.mask |= LINT_NOTE;
				}
				else if(!strcmp(optarg, "all"))
				{
					app.show |= (LINT_WARN | LINT_NOTE);
					app.mask |= (LINT_WARN | LINT_NOTE);
				}

				else if(!strcmp(optarg, "nowarn"))
				{
					app.show &= ~LINT_WARN;
					app.mask &= ~LINT_WARN;
				}
				else if(!strcmp(optarg, "nonote"))
				{
					app.show &= ~LINT_NOTE;
					app.mask &= ~LINT_NOTE;
				}
				else if(!strcmp(optarg, "noall"))
				{
					app.show &= ~(LINT_WARN | LINT_NOTE);
					app.mask &= ~(LINT_WARN | LINT_NOTE);
				}

				break;
			case '?':
#ifdef ENABLE_ONLINE_TESTS
				if( (optopt == 'S') || (optopt == 'E') || (optopt == 'g') )
#else
				if( (optopt == 'S') || (optopt == 'E') )
#endif
					fprintf(stderr, "Option `-%c' requires an argument.\n", optopt);
				else if(isprint(optopt))
					fprintf(stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
				return -1;
			default:
				return -1;
		}
	}

	if(optind == argc) // no URI given
	{
		_usage(argv);
		return -1;
	}

	if(!app.quiet)
	{
		_header(argv);
	}

#ifdef ENABLE_ONLINE_TESTS
	app.curl = curl_easy_init();
	if(!app.curl)
		return -1;
#endif

	app.world = lilv_world_new();
	if(!app.world)
		return -1;

	mapper_t *mapper = mapper_new(8192, 0, NULL, NULL, NULL, NULL);
	if(!mapper)
		return -1;

	_map_uris(&app);
	lilv_world_load_all(app.world);
	_load_include_dirs(&app);

	LV2_URID_Map *map = mapper_get_map(mapper);
	LV2_URID_Unmap *unmap = mapper_get_unmap(mapper);
	LV2_Worker_Schedule sched = {
		.handle = &app,
		.schedule_work = _sched
	};
	LV2_Log_Log log = {
		.handle = &app,
		.printf = _printf,
		.vprintf = _vprintf
	};
	LV2_State_Make_Path mkpath = {
		.handle = &app,
		.path = _mkpath
	};
	LV2_Resize_Port_Resize rsz = {
		.data = &app,
		.resize = _resize
	};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	LV2_URI_Map_Feature urimap = {
		.callback_data = map,
		.uri_to_id = _uri_to_id
	};
#pragma GCC diagnostic pop
	LV2_Inline_Display queue_draw = {
		.handle = &app,
		.queue_draw = _queue_draw
	};

	const LV2_URID atom_Float = map->map(map->handle, LV2_ATOM__Float);
	const LV2_URID atom_Int = map->map(map->handle, LV2_ATOM__Int);
	const LV2_URID param_sampleRate = map->map(map->handle, LV2_PARAMETERS__sampleRate);
	const LV2_URID ui_updateRate = map->map(map->handle, LV2_UI__updateRate);
	const LV2_URID bufsz_minBlockLength = map->map(map->handle, LV2_BUF_SIZE__minBlockLength);
	const LV2_URID bufsz_maxBlockLength = map->map(map->handle, LV2_BUF_SIZE__maxBlockLength);
	const LV2_URID bufsz_nominalBlockLength = map->map(map->handle, LV2_BUF_SIZE_PREFIX"nominalBlockLength");
	const LV2_URID bufsz_sequenceSize = map->map(map->handle, LV2_BUF_SIZE__sequenceSize);

	const float param_sample_rate = 48000.f;
	const float ui_update_rate = 25.f;
	const int32_t bufsz_min_block_length = 256;
	const int32_t bufsz_max_block_length = 256;
	const int32_t bufsz_nominal_block_length = 256;
	const int32_t bufsz_sequence_size = 2048;

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

	const LV2_Options_Option opts_minBlockLength = {
		.key = bufsz_minBlockLength,
		.size = sizeof(int32_t),
		.type = atom_Int,
		.value = &bufsz_min_block_length
	};

	const LV2_Options_Option opts_maxBlockLength = {
		.key = bufsz_maxBlockLength,
		.size = sizeof(int32_t),
		.type = atom_Int,
		.value = &bufsz_max_block_length
	};

	const LV2_Options_Option opts_nominalBlockLength = {
		.key = bufsz_nominalBlockLength,
		.size = sizeof(int32_t),
		.type = atom_Int,
		.value = &bufsz_nominal_block_length
	};

	const LV2_Options_Option opts_sequenceSize = {
		.key = bufsz_sequenceSize,
		.size = sizeof(int32_t),
		.type = atom_Int,
		.value = &bufsz_sequence_size
	};

	const LV2_Options_Option opts_sentinel = {
		.key = 0,
		.value =NULL
	};

#define MAX_OPTS  7
	LV2_Options_Option opts [MAX_OPTS];

	const LV2_Feature feat_map = {
		.URI = LV2_URID__map,
		.data = map
	};
	const LV2_Feature feat_unmap = {
		.URI = LV2_URID__unmap,
		.data = unmap
	};
	const LV2_Feature feat_sched = {
		.URI = LV2_WORKER__schedule,
		.data = &sched
	};
	const LV2_Feature feat_log = {
		.URI = LV2_LOG__log,
		.data = &log
	};
	const LV2_Feature feat_mkpath = {
		.URI = LV2_STATE__makePath,
		.data = &mkpath
	};
	const LV2_Feature feat_rsz = {
		.URI = LV2_RESIZE_PORT__resize,
		.data = &rsz
	};
	const LV2_Feature feat_opts = {
		.URI = LV2_OPTIONS__options,
		.data = opts
	};
	const LV2_Feature feat_urimap = {
		.URI = LV2_URI_MAP_URI,
		.data = &urimap
	};

	const LV2_Feature feat_islive = {
		.URI = LV2_CORE__isLive
	};
	const LV2_Feature feat_inplacebroken = {
		.URI = LV2_CORE__inPlaceBroken
	};
	const LV2_Feature feat_hardrtcapable = {
		.URI = LV2_CORE__hardRTCapable
	};
	const LV2_Feature feat_supportsstrictbounds = {
		.URI = LV2_PORT_PROPS__supportsStrictBounds
	};
	const LV2_Feature feat_boundedblocklength = {
		.URI = LV2_BUF_SIZE__boundedBlockLength
	};
	const LV2_Feature feat_fixedblocklength = {
		.URI = LV2_BUF_SIZE__fixedBlockLength
	};
	const LV2_Feature feat_powerof2blocklength = {
		.URI = LV2_BUF_SIZE__powerOf2BlockLength
	};
	const LV2_Feature feat_coarseblocklength = {
		.URI = LV2_BUF_SIZE_PREFIX"coarseBlockLength"
	};
	const LV2_Feature feat_loaddefaultstate = {
		.URI = LV2_STATE__loadDefaultState
	};
	const LV2_Feature feat_threadsaferestore = {
		.URI = LV2_STATE_PREFIX"threadSafeRestore"
	};
	const LV2_Feature feat_idispqueuedraw = {
		.URI = LV2_INLINEDISPLAY__queue_draw,
		.data = &queue_draw
	};

	int ret = 0;
	const LilvPlugin *plugins = lilv_world_get_all_plugins(app.world);
	if(plugins)
	{
		for(int i=optind; i<argc; i++)
		{
			app.plugin_uri = argv[i];
			LilvNode *plugin_uri_node = lilv_new_uri(app.world, app.plugin_uri);
			if(plugin_uri_node)
			{
				app.plugin = lilv_plugins_get_by_uri(plugins, plugin_uri_node);
				if(app.plugin)
				{
#define MAX_FEATURES 20
					const LV2_Feature *features [MAX_FEATURES];
					bool requires_bounded_block_length = false;

					// populate feature list
					{
						int f = 0;

						LilvNodes *required_features = lilv_plugin_get_required_features(app.plugin);
						if(required_features)
						{
							LILV_FOREACH(nodes, itr, required_features)
							{
								const LilvNode *feature = lilv_nodes_get(required_features, itr);

								if(lilv_node_equals(feature, app.uris.urid_map))
									features[f++] = &feat_map;
								else if(lilv_node_equals(feature, app.uris.urid_unmap))
									features[f++] = &feat_unmap;
								else if(lilv_node_equals(feature, app.uris.work_schedule))
									features[f++] = &feat_sched;
								else if(lilv_node_equals(feature, app.uris.log_log))
									features[f++] = &feat_log;
								else if(lilv_node_equals(feature, app.uris.state_makePath))
									features[f++] = &feat_mkpath;
								else if(lilv_node_equals(feature, app.uris.rsz_resize))
									features[f++] = &feat_rsz;
								else if(lilv_node_equals(feature, app.uris.opts_options))
									features[f++] = &feat_opts;
								else if(lilv_node_equals(feature, app.uris.uri_map))
									features[f++] = &feat_urimap;
								else if(lilv_node_equals(feature, app.uris.lv2_isLive))
									features[f++] = &feat_islive;
								else if(lilv_node_equals(feature, app.uris.lv2_inPlaceBroken))
									features[f++] = &feat_inplacebroken;
								else if(lilv_node_equals(feature, app.uris.lv2_hardRTCapable))
									features[f++] = &feat_hardrtcapable;
								else if(lilv_node_equals(feature, app.uris.pprops_supportsStrictBounds))
									features[f++] = &feat_supportsstrictbounds;
								else if(lilv_node_equals(feature, app.uris.bufsz_boundedBlockLength))
								{
									features[f++] = &feat_boundedblocklength;
									requires_bounded_block_length = true;
								}
								else if(lilv_node_equals(feature, app.uris.bufsz_fixedBlockLength))
									features[f++] = &feat_fixedblocklength;
								else if(lilv_node_equals(feature, app.uris.bufsz_powerOf2BlockLength))
									features[f++] = &feat_powerof2blocklength;
								else if(lilv_node_equals(feature, app.uris.bufsz_coarseBlockLength))
									features[f++] = &feat_coarseblocklength;
								else if(lilv_node_equals(feature, app.uris.state_loadDefaultState))
									features[f++] = &feat_loaddefaultstate;
								else if(lilv_node_equals(feature, app.uris.state_threadSafeRestore))
									features[f++] = &feat_threadsaferestore;
								else if(lilv_node_equals(feature, app.uris.idisp_queue_draw))
									features[f++] = &feat_idispqueuedraw;
								else
								{
									//FIXME unknown feature
								}
							}
							lilv_nodes_free(required_features);
						}

						features[f++] = NULL; // sentinel
						assert(f <= MAX_FEATURES);
					}

					// populate required option list
					{
						unsigned n_opts = 0;
						bool requires_min_block_length = false;
						bool requires_max_block_length = false;

						LilvNodes *required_options = lilv_plugin_get_value(app.plugin, app.uris.opts_requiredOption);
						if(required_options)
						{
							LILV_FOREACH(nodes, itr, required_options)
							{
								const LilvNode *option = lilv_nodes_get(required_options, itr);

								if(lilv_node_equals(option, app.uris.param_sampleRate))
								{
									opts[n_opts++] = opts_sampleRate;
								}
								else if(lilv_node_equals(option, app.uris.bufsz_minBlockLength))
								{
									opts[n_opts++] = opts_minBlockLength;
									requires_min_block_length = true;
								}
								else if(lilv_node_equals(option, app.uris.bufsz_maxBlockLength))
								{
									opts[n_opts++] = opts_maxBlockLength;
									requires_max_block_length = true;
								}
								else if(lilv_node_equals(option, app.uris.bufsz_nominalBlockLength))
								{
									opts[n_opts++] = opts_nominalBlockLength;
								}
								else if(lilv_node_equals(option, app.uris.bufsz_sequenceSize))
								{
									opts[n_opts++] = opts_sequenceSize;
								}
								else if(lilv_node_equals(option, app.uris.ui_updateRate))
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

						// handle bufsz:boundedBlockLength feature which activates options itself
						if(requires_bounded_block_length)
						{
							if(!requires_min_block_length) // was not explicitely required
								opts[n_opts++] = opts_minBlockLength;

							if(!requires_max_block_length) // was not explicitely required
								opts[n_opts++] = opts_maxBlockLength;
						}

						opts[n_opts++] = opts_sentinel; // sentinel
						assert(n_opts <= MAX_OPTS);
					}

#ifdef ENABLE_ONLINE_TESTS
					if(app.mailto)
					{
						app.mail = calloc(1, sizeof(char));
					}
#endif

					lv2lint_printf(&app, "%s<%s>%s\n",
						colors[app.atty][ANSI_COLOR_BOLD],
						lilv_node_as_uri(lilv_plugin_get_uri(app.plugin)),
						colors[app.atty][ANSI_COLOR_RESET]);

					app.instance = lilv_plugin_instantiate(app.plugin, param_sample_rate, features);

					if(app.instance)
					{
						app.work_iface = lilv_instance_get_extension_data(app.instance, LV2_WORKER__interface);
						app.idisp_iface = lilv_instance_get_extension_data(app.instance, LV2_INLINEDISPLAY__interface);
						app.state_iface = lilv_instance_get_extension_data(app.instance, LV2_STATE__interface);
						app.opts_iface = lilv_instance_get_extension_data(app.instance, LV2_OPTIONS__interface);

						const bool has_load_default = lilv_plugin_has_feature(app.plugin,
							app.uris.state_loadDefaultState);
						if(has_load_default)
						{
							const LilvNode *pset = lilv_plugin_get_uri(app.plugin);

							lilv_world_load_resource(app.world, pset);

							LilvState *state = lilv_state_new_from_world(app.world, map, pset);
							if(state)
							{
								lilv_state_restore(state, app.instance, _state_set_value, &app,
									LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE, NULL); //FIXME features

								lilv_state_free(state);
							}

							lilv_world_unload_resource(app.world, pset);
						}
					}

					if(!test_plugin(&app))
					{
#ifdef ENABLE_ONLINE_TESTS // only print mailto strings if errors were encountered
						if(app.mailto && app.mail)
						{
							char *subj;
							unsigned minor_version = 0;
							unsigned micro_version = 0;

							LilvNode *minor_version_nodes = lilv_plugin_get_value(app.plugin , app.uris.lv2_minorVersion);
							if(minor_version_nodes)
							{
								const LilvNode *minor_version_node = lilv_nodes_get_first(minor_version_nodes);
								if(minor_version_node && lilv_node_is_int(minor_version_node))
								{
									minor_version = lilv_node_as_int(minor_version_node);
								}

								lilv_nodes_free(minor_version_nodes);
							}

							LilvNode *micro_version_nodes = lilv_plugin_get_value(app.plugin , app.uris.lv2_microVersion);
							if(micro_version_nodes)
							{
								const LilvNode *micro_version_node = lilv_nodes_get_first(micro_version_nodes);
								if(micro_version_node && lilv_node_is_int(micro_version_node))
								{
									micro_version = lilv_node_as_int(micro_version_node);
								}

								lilv_nodes_free(micro_version_nodes);
							}

							if(asprintf(&subj, "[%s "LV2LINT_VERSION"] bug report for <%s> version %u.%u",
								argv[0], app.plugin_uri, minor_version, micro_version) != -1)
							{
								char *subj_esc = curl_easy_escape(app.curl, subj, strlen(subj));
								if(subj_esc)
								{
									char *greet_esc = curl_easy_escape(app.curl, app.greet, strlen(app.greet));
									if(greet_esc)
									{
										char *body_esc = curl_easy_escape(app.curl, app.mail, strlen(app.mail));
										if(body_esc)
										{
											LilvNode *email_node = lilv_plugin_get_author_email(app.plugin);
											const char *email = email_node && lilv_node_is_uri(email_node)
												? lilv_node_as_uri(email_node)
												: "mailto:unknown@example.com";

											fprintf(stdout, "%s?subject=%s&body=%s%s\n",
												email, subj_esc, greet_esc, body_esc);

											if(email_node)
											{
												lilv_node_free(email_node);
											}

											curl_free(body_esc);
										}

										curl_free(greet_esc);
									}

									curl_free(subj_esc);
								}

								free(subj);
							}
						}
#endif

						ret += 1;
					}

#ifdef ENABLE_ONLINE_TESTS
					if(app.mail)
					{
						free(app.mail);
						app.mail = NULL;
					}
#endif

					if(app.instance)
					{
						lilv_instance_free(app.instance);
						app.instance = NULL;
						app.work_iface = NULL;
						app.idisp_iface = NULL;
						app.state_iface= NULL;
						app.opts_iface = NULL;
					}

					app.plugin = NULL;

				}
				else
				{
					ret += 1;
				}
			}
			else
			{
				ret += 1;
			}
			lilv_node_free(plugin_uri_node);
		}
	}
	else
	{
		ret = -1;
	}

	_unmap_uris(&app);
	_free_urids(&app);
	_free_include_dirs(&app);
	_free_whitelist_tests(&app);
#ifdef ENABLE_ELF_TESTS
	_free_whitelist_symbols(&app);
	_free_whitelist_libs(&app);
#endif
	mapper_free(mapper);

	lilv_world_free(app.world);

#ifdef ENABLE_ONLINE_TESTS
	curl_easy_cleanup(app.curl);
#endif

	return ret;
}

int
lv2lint_vprintf(app_t *app, const char *fmt, va_list args)
{
#ifdef ENABLE_ONLINE_TESTS
	if(app->mailto)
	{
		char *buf = NULL;
		int len;

		if( (len = vasprintf(&buf, fmt, args)) != -1)
		{
			len += strlen(app->mail);
			app->mail = realloc(app->mail, len + 1);

			if(app->mail)
			{
				app->mail = strncat(app->mail, buf, len + 1);
			}

			free(buf);
		}
	}
	else
#else
	(void)app;
#endif
	{
		vfprintf(stdout, fmt, args);
	}

	return 0;
}

int
lv2lint_printf(app_t *app, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);

	const int ret = lv2lint_vprintf(app, fmt, args);

	va_end(args);

	return ret;
}

static void
_escape_markup(char *docu)
{
	char *wrp = docu;
	bool tag = false;
	bool amp = false;
	bool sep = false;

	for(const char *rdp = docu; *rdp != '\0'; rdp++)
	{
		switch(*rdp)
		{
			case '<':
			{
				tag = true;
			} continue;
			case '>':
			{
				if(tag)
				{
					tag = false;
					continue;
				}
			} break;

			case '&':
			{
				amp = true;
			} continue;
			case ';':
			{
				if(amp)
				{
					amp = false;
					continue;
				}
			} break;

			case ' ':
			{
				if(sep) // escape double spaces
				{
					continue;
				}

				sep = true;
			} break;

			default:
			{
				sep = false;
			} break;
		}

		if(tag || amp)
		{
			continue;
		}

		*wrp++ = *rdp;
	}

	*wrp = '\0';
}

static void
_report_head(app_t *app, const char *label, ansi_color_t col, const test_t *test)
{
	lv2lint_printf(app, "    [%s%s%s]  %s\n",
		colors[app->atty][col], label, colors[app->atty][ANSI_COLOR_RESET], test->id);
}

static void
_report_body(app_t *app, const char *label, ansi_color_t col, const test_t *test,
	const ret_t *ret, const char *repl, char *docu)
{
	_report_head(app, label, col, test);

	lv2lint_printf(app, "              %s\n", repl ? repl : ret->msg);

	if(docu)
	{
		_escape_markup(docu);

		const char *sep = "\n";
		for(char *docup = docu, *ptr = strsep(&docup, sep);
			ptr;
			ptr = strsep(&docup, sep) )
		{
			lv2lint_printf(app, "                %s\n", ptr);
		}
	}

	lv2lint_printf(app, "              seeAlso: <%s>\n", ret->uri);
}

void
lv2lint_report(app_t *app, const test_t *test, res_t *res, bool show_passes, bool *flag)
{
	const ret_t *ret = res->ret;

	if(ret)
	{
		char *repl = NULL;

		if(res->urn)
		{
			if(strstr(ret->msg, "%s"))
			{
				if(asprintf(&repl, ret->msg, res->urn) == -1)
					repl = NULL;
			}

			free(res->urn);
		}

		char *docu = NULL;

		if(app->debug)
		{
			if(ret->dsc)
			{
				docu = strdup(ret->dsc);
			}
			else
			{
				LilvNode *subj_node = ret->uri ? lilv_new_uri(app->world, ret->uri) : NULL;
				if(subj_node)
				{
					LilvNode *docu_node = lilv_world_get(app->world, subj_node, app->uris.lv2_documentation, NULL);
					if(docu_node)
					{
						if(lilv_node_is_string(docu_node))
						{
							docu = strdup(lilv_node_as_string(docu_node));
						}

						lilv_node_free(docu_node);
					}

					lilv_node_free(subj_node);
				}
			}
		}

		const lint_t lnt = lv2lint_extract(app, ret);
		switch(lnt & app->show)
		{
			case LINT_FAIL:
				_report_body(app, "FAIL", ANSI_COLOR_RED, test, ret, repl, docu);
				break;
			case LINT_WARN:
				_report_body(app, "WARN", ANSI_COLOR_YELLOW, test, ret, repl, docu);
				break;
			case LINT_NOTE:
				_report_body(app, "NOTE", ANSI_COLOR_CYAN, test, ret, repl, docu);
				break;
		}

		if(docu)
		{
			free(docu);
		}

		if(repl)
		{
			free(repl);
		}

		if(flag && *flag)
		{
			*flag = (lnt & app->mask) ? false : true;
		}
	}
	else if(show_passes)
	{
		_report_head(app, "PASS", ANSI_COLOR_GREEN, test);
	}
}

lint_t
lv2lint_extract(app_t *app, const ret_t *ret)
{
	if(!ret)
	{
		return LINT_NONE;
	}

	return app->pck && (ret->pck != LINT_NONE)
		? ret->pck
		: ret->lnt;
}
