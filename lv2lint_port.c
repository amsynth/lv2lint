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

#include <math.h>

#include <lv2lint.h>

#include <lv2/event/event.h>
#include <lv2/urid/urid.h>
#include <lv2/morph/morph.h>
#include <lv2/port-groups/port-groups.h>
#include <lv2/units/units.h>

static const ret_t *
_test_class(app_t *app)
{
	static const ret_t ret_class_not_valid = {
		.lnt = LINT_FAIL,
		.msg = "lv2:Port class <%s> not valid",
		.uri = LV2_CORE__Port,
		.dsc = "This port class likely is not defined anywhere."
	};

	const ret_t *ret = NULL;

	LilvNodes *class = lilv_world_find_nodes(app->world,
		NULL, NODE(app, RDFS__subClassOf), NODE(app, CORE__Port));
	if(class)
	{
		const LilvNodes *supported= lilv_port_get_classes(app->plugin, app->port);
		if(supported)
		{
			LILV_FOREACH(nodes, itr, supported)
			{
				const LilvNode *node = lilv_nodes_get(supported, itr);

				if(!lilv_nodes_contains(class, node))
				{
					*app->urn = lv2lint_node_as_uri_strdup(node);
					ret = &ret_class_not_valid;
					break;
				}
			}
		}

		lilv_nodes_free(class);
	}

	return ret;
}

static const ret_t *
_test_properties(app_t *app)
{
	static const ret_t ret_properties_not_valid = {
		.lnt = LINT_FAIL,
		.msg = "lv2:portProperty <%s> not valid",
		.uri = LV2_CORE__portProperty,
		.dsc = "This property likely is not defined anywhere."
	};

	const ret_t *ret = NULL;

	LilvNodes *properties = lilv_world_find_nodes(app->world,
		NULL, NODE(app, RDF__type), NODE(app, CORE__PortProperty));
	if(properties)
	{
		LilvNodes *supported = lilv_port_get_properties(app->plugin, app->port);
		if(supported)
		{
			LILV_FOREACH(nodes, itr, supported)
			{
				const LilvNode *node = lilv_nodes_get(supported, itr);

				if(!lilv_nodes_contains(properties, node))
				{
					*app->urn = lv2lint_node_as_uri_strdup(node);
					ret = &ret_properties_not_valid;
					break;
				}
			}

			lilv_nodes_free(supported);
		}

		lilv_nodes_free(properties);
	}

	return ret;
}

static inline const ret_t *
_test_num(app_t *app, LilvNode *node, bool is_integer, bool is_toggled,
	float *val, const char *uri)
{
	static const ret_t ret_num_not_found = {
		.lnt = LINT_WARN,
		.msg = "number not found for <%s>",
		.uri = LV2_CORE__Port,
		.dsc = NULL
	},
	ret_num_not_an_int = {
		.lnt = LINT_NOTE,
		.msg = "number not an integer for <%s>",
		.uri = LV2_CORE__Port,
		.dsc = NULL
	},
	ret_num_not_a_float = {
		.lnt = LINT_NOTE,
		.msg = "number not a float for <%s>",
		.uri = LV2_CORE__Port,
		.dsc = NULL
	},
	ret_num_not_a_bool = {
		.lnt = LINT_NOTE,
		.msg = "number not a bool <%s>",
		.uri = LV2_CORE__Port,
		.dsc = NULL
	},
	ret_num_not_a_whole_value = {
		.lnt = LINT_WARN,
		.msg = "number has no whole value <%s>",
		.uri = LV2_CORE__Port,
		.dsc = NULL
	},
	ret_num_not_a_boolean_value = {
		.lnt = LINT_WARN,
		.msg = "number has no boolean value <%s>",
		.uri = LV2_CORE__Port,
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	if(node)
	{
		if(lilv_node_is_int(node))
		{
			*val = lilv_node_as_int(node);
		}
		else if(lilv_node_is_float(node))
		{
			*val = lilv_node_as_float(node);
		}
		else if(lilv_node_is_bool(node))
		{
			*val = lilv_node_as_bool(node) ? 1.f : 0.f;
		}

		if(is_integer)
		{
			if(lilv_node_is_int(node))
			{
				// OK
			}
			else if(lilv_node_is_float(node))
			{
				if(rintf(lilv_node_as_float(node)) == lilv_node_as_float(node))
				{
					*app->urn = lv2lint_strdup(uri);
					ret = &ret_num_not_an_int;
				}
				else
				{
					*app->urn = lv2lint_strdup(uri);
					ret = &ret_num_not_a_whole_value;
				}
			}
			else // bool
			{
				*app->urn = lv2lint_strdup(uri);
				ret = &ret_num_not_an_int;
			}
		}
		else if(is_toggled)
		{
			if(lilv_node_is_int(node))
			{
				if( (lilv_node_as_int(node) == 0) || (lilv_node_as_int(node) == 1) )
				{
					*app->urn = lv2lint_strdup(uri);
					ret = &ret_num_not_a_bool;
				}
				else
				{
					*app->urn = lv2lint_strdup(uri);
					ret = &ret_num_not_a_boolean_value;
				}
			}
			else if(lilv_node_is_float(node))
			{
				if( (lilv_node_as_float(node) == 0.f) || (lilv_node_as_float(node) == 1.f) )
				{
					*app->urn = lv2lint_strdup(uri);
					ret = &ret_num_not_a_bool;
				}
				else
				{
					*app->urn = lv2lint_strdup(uri);
					ret = &ret_num_not_a_boolean_value;
				}
			}
			else // bool
			{
				// OK
			}
		}
		else if(!lilv_node_is_float(node))
		{
			*app->urn = lv2lint_strdup(uri);
			ret = &ret_num_not_a_float;
		}

		lilv_node_free(node);
	}
	else // !node
	{
		*app->urn = lv2lint_strdup(uri);
		ret = &ret_num_not_found;
	}

	return ret;
}

static const ret_t *
_test_default(app_t *app)
{
	const ret_t *ret = NULL;

	app->dflt.f32 = 0.f; // fall-back

	const bool is_integer = lilv_port_has_property(app->plugin, app->port, NODE(app, CORE__integer));
	const bool is_toggled = lilv_port_has_property(app->plugin, app->port, NODE(app, CORE__toggled));

	if(  (lilv_port_is_a(app->plugin, app->port, NODE(app, CORE__ControlPort))
			|| lilv_port_is_a(app->plugin, app->port, NODE(app, CORE__CVPort)))
		&& lilv_port_is_a(app->plugin, app->port, NODE(app, CORE__InputPort)) )
	{
		LilvNode *default_node = lilv_port_get(app->plugin, app->port, NODE(app, CORE__default));
		ret = _test_num(app, default_node, is_integer, is_toggled, &app->dflt.f32,
			LV2_CORE__default);
		//lilv_node_free(default_node);
	}

	return ret;
}

static const ret_t *
_test_minimum(app_t *app)
{
	const ret_t *ret = NULL;

	app->min.f32 = 0.f; // fall-back

	const bool is_integer = lilv_port_has_property(app->plugin, app->port, NODE(app, CORE__integer));
	const bool is_toggled = lilv_port_has_property(app->plugin, app->port, NODE(app, CORE__toggled));

	if(  (lilv_port_is_a(app->plugin, app->port, NODE(app, CORE__ControlPort))
			|| lilv_port_is_a(app->plugin, app->port, NODE(app, CORE__CVPort)))
		&& lilv_port_is_a(app->plugin, app->port, NODE(app, CORE__InputPort))
		&& !is_toggled )
	{
		LilvNode *minimum_node = lilv_port_get(app->plugin, app->port, NODE(app, CORE__minimum));
		ret = _test_num(app, minimum_node, is_integer, is_toggled, &app->min.f32,
			LV2_CORE__minimum);
		//lilv_node_free(minimum_node);
	}

	return ret;
}

static const ret_t *
_test_maximum(app_t *app)
{
	const ret_t *ret = NULL;

	app->max.f32 = 1.f; // fall-back

	const bool is_integer = lilv_port_has_property(app->plugin, app->port, NODE(app, CORE__integer));
	const bool is_toggled = lilv_port_has_property(app->plugin, app->port, NODE(app, CORE__toggled));

	if(  (lilv_port_is_a(app->plugin, app->port, NODE(app, CORE__ControlPort))
			|| lilv_port_is_a(app->plugin, app->port, NODE(app, CORE__CVPort)))
		&& lilv_port_is_a(app->plugin, app->port, NODE(app, CORE__InputPort))
		&& !is_toggled )
	{
		LilvNode *maximum_node = lilv_port_get(app->plugin, app->port, NODE(app, CORE__maximum));
		ret = _test_num(app, maximum_node, is_integer, is_toggled, &app->max.f32,
			LV2_CORE__maximum);
		//lilv_node_free(maximum_node);
	}

	return ret;
}

static const ret_t *
_test_range(app_t *app)
{
	static const ret_t ret_range = {
		.lnt = LINT_FAIL,
		.msg = "range invalid (min <= default <= max)",
		.uri = LV2_CORE__Port,
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	if(  lilv_port_is_a(app->plugin, app->port, NODE(app, CORE__ControlPort))
			|| lilv_port_is_a(app->plugin, app->port, NODE(app, CORE__CVPort)) )
	{
		float min = app->min.f32;
		float max = app->max.f32;
		if(lilv_port_has_property(app->plugin, app->port, NODE(app, CORE__sampleRate)))
		{
			min *= 44100.0;
			max *= 44100.0;
		}

		if( !( (min <= app->dflt.f32) && (app->dflt.f32 <= max) ) )
		{
			ret = &ret_range;
		}
	}

	return ret;
}

static const ret_t *
_test_atom_port(app_t *app)
{
	static const ret_t ret_atom_port_requires_urid_map = {
		.lnt = LINT_FAIL,
		.msg = "atom:AtomPort requires urid:map feature",
		.uri = LV2_URID__map,
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	if(lilv_port_is_a(app->plugin, app->port, NODE(app, ATOM__AtomPort)))
	{
		const bool has_urid_map = lilv_plugin_has_feature(app->plugin,
			NODE(app, URID__map));

		if(!has_urid_map)
		{
			ret = &ret_atom_port_requires_urid_map;
		}
	}

	return ret;
}

static const ret_t *
_test_event_port(app_t *app)
{
	static const ret_t ret_event_port_deprecated = {
		.lnt = LINT_FAIL,
		.msg = "lv2:EventPort is deprecated, use atom:AtomPort instead",
		.uri = LV2_EVENT__EventPort,
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	if(lilv_port_is_a(app->plugin, app->port, NODE(app, EVENT__EventPort)))
	{
		ret = &ret_event_port_deprecated;
	}

	return ret;
}

static const ret_t *
_test_morph_port(app_t *app)
{
	static const ret_t ret_morph_port_not_found = {
		.lnt = LINT_FAIL,
		.msg = "morph port not found",
		.uri = LV2_MORPH__MorphPort,
		.dsc = NULL
	},
	ret_morph_supported_types_not_found = {
		.lnt = LINT_FAIL,
		.msg = "supported types for morph port not found",
		.uri = LV2_MORPH__supportsType,
		.dsc = NULL
	},
	ret_morph_supported_types_not_enough = {
		.lnt = LINT_FAIL,
		.msg = "not enough supported types found",
		.uri = LV2_MORPH__supportsType,
		.dsc = NULL
	},
	ret_morph_default_not_found = {
		.lnt = LINT_FAIL,
		.msg = "default port type not found",
		.uri = LV2_MORPH__MorphPort,
		.dsc = NULL
	};

	LilvNodes *morph_supported_types = lilv_port_get_value(app->plugin, app->port,
		NODE(app, MORPH__supportsType));
	const unsigned n_morph_supported_types = morph_supported_types
		? lilv_nodes_size(morph_supported_types)
		: 0;

	const bool is_morph_port = lilv_port_is_a(app->plugin, app->port,
		NODE(app, MORPH__MorphPort));
	const bool is_auto_morph_port = lilv_port_is_a(app->plugin, app->port,
		NODE(app, MORPH__AutoMorphPort));
	const bool is_any_morph_port = is_morph_port || is_auto_morph_port;

	const bool is_control_port = lilv_port_is_a(app->plugin, app->port,
		NODE(app, CORE__ControlPort));
	const bool is_audio_port = lilv_port_is_a(app->plugin, app->port,
		NODE(app, CORE__AudioPort));
	const bool is_cv_port = lilv_port_is_a(app->plugin, app->port,
		NODE(app, CORE__CVPort));
	const bool is_atom_port = lilv_port_is_a(app->plugin, app->port,
		NODE(app, ATOM__AtomPort));
	const bool is_event_port = lilv_port_is_a(app->plugin, app->port,
		NODE(app, EVENT__EventPort));

	const ret_t *ret = NULL;

	if(is_any_morph_port || n_morph_supported_types)
	{
		if(!is_any_morph_port)
		{
			ret = &ret_morph_port_not_found;;
		}
		else if(!n_morph_supported_types)
		{
			ret = &ret_morph_supported_types_not_found;
		}
		else if(n_morph_supported_types < 2)
		{
			ret = &ret_morph_supported_types_not_enough;;
		}
		else if(!is_control_port || !is_audio_port || !is_cv_port || !is_atom_port || !is_event_port)
		{
			ret = &ret_morph_default_not_found;
		}
	}

	return ret;
}

static const ret_t *
_test_comment(app_t *app)
{
	static const ret_t ret_comment_not_found = {
		.lnt = LINT_NOTE,
		.msg = "rdfs:comment not found",
		.uri = LILV_NS_RDFS"comment",
		.dsc = "Adding a description helps the user to better undetstand this port."
	},
	ret_comment_not_a_string = {
		.lnt = LINT_FAIL,
		.msg = "rdfs:comment not a string",
		.uri = LILV_NS_RDFS"comment",
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	LilvNode *comment = lilv_port_get(app->plugin, app->port, NODE(app, RDFS__comment));
	if(comment)
	{
		if(!lilv_node_is_string(comment))
		{
			ret = &ret_comment_not_a_string;
		}

		lilv_node_free(comment);
	}
	else
	{
		ret = &ret_comment_not_found;
	}

	return ret;
}

static const ret_t *
_test_group(app_t *app)
{
	static const ret_t ret_group_not_found = {
		.lnt = LINT_NOTE,
		.msg = "pg:group not found",
		.uri = LV2_PORT_GROUPS__group,
		.dsc = "Subsumming ports into groups helps to draw more comprehensible "
			"generic UIs."
	},
	ret_group_not_a_uri = {
		.lnt = LINT_FAIL,
		.msg = "pg:group not a URI",
		.uri = LV2_PORT_GROUPS__group,
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	LilvNode *group = lilv_port_get(app->plugin, app->port, NODE(app, PORT_GROUPS__group));
	if(group)
	{
		if(!lilv_node_is_uri(group))
		{
			ret = &ret_group_not_a_uri;
		}

		lilv_node_free(group);
	}
	else
	{
		ret = &ret_group_not_found;
	}

	return ret;
}

static const ret_t *
_test_unit(app_t *app)
{
	static const ret_t ret_units_unit_not_found = {
		.lnt = LINT_NOTE,
		.msg = "units:unit not found",
		.uri = LV2_UNITS__unit,
		.dsc = "Adding units to controls helps the user to put things in perspective."
	},
	ret_units_unit_not_a_uri_or_object = {
		.lnt = LINT_FAIL,
		.msg = "units_unit not a URI or object",
		.uri = LV2_UNITS__unit,
		.dsc = NULL
	};

	const ret_t *ret = NULL;


	if(  lilv_port_is_a(app->plugin, app->port, NODE(app, CORE__ControlPort))
		|| lilv_port_is_a(app->plugin, app->port, NODE(app, CORE__CVPort)) )
	{
		LilvNode *unit = lilv_port_get(app->plugin, app->port, NODE(app, UNITS__unit));
		if(unit)
		{
			if(  !lilv_node_is_uri(unit)
				&& !lilv_world_ask(app->world, unit, NODE(app, RDF__type), NODE(app, UNITS__Unit)) )
			{
				ret = &ret_units_unit_not_a_uri_or_object;
			}

			lilv_node_free(unit);
		}
		else
		{
			ret = &ret_units_unit_not_found;
		}
	}

	return ret;
}

static const ret_t *
_test_symbol(app_t *app)
{
	static const ret_t ret_not_unique = {
		.lnt = LINT_FAIL,
		.msg = "lv2:symbol not unique",
		.uri = LV2_CORE__symbol,
		.dsc = "Port symbols MUST be unique."
	};
	const ret_t *ret = NULL;

	const LilvNode *symbol1 = lilv_port_get_symbol(app->plugin, app->port);

	const uint32_t num_ports = lilv_plugin_get_num_ports(app->plugin);
	for(unsigned i=0; i<num_ports; i++)
	{
		const LilvPort *port = lilv_plugin_get_port_by_index(app->plugin, i);

		if(port == app->port)
		{
			continue; // skip self
		}

		const LilvNode *symbol2 = lilv_port_get_symbol(app->plugin, port);

		if(lilv_node_equals(symbol1, symbol2))
		{
			ret = &ret_not_unique;

			break;
		}
	}

	return ret;
}

static const ret_t *
_test_scale_points(app_t *app)
{
	static const ret_t ret_not_unique_val = {
		.lnt = LINT_FAIL,
		.msg = "lv2:scalePoint has not unique values",
		.uri = LV2_CORE__scalePoint,
		.dsc = "Scale point values SHOULD be unique."
	},
	ret_not_unique_lbl = {
		.lnt = LINT_WARN,
		.msg = "lv2:scalePoint has not unique labels",
		.uri = LV2_CORE__scalePoint,
		.dsc = "Scale point labels SHOULD be unique."
	};
	const ret_t *ret = NULL;

	LilvScalePoints *sps = lilv_port_get_scale_points(app->plugin, app->port);
	if(sps)
	{
		LILV_FOREACH(scale_points, iter1, sps)
		{
			const LilvScalePoint *sp1 = lilv_scale_points_get(sps, iter1);
			const LilvNode *val1 = lilv_scale_point_get_value(sp1);
			const LilvNode *lbl1 = lilv_scale_point_get_label(sp1);

			LILV_FOREACH(scale_points, iter2, sps)
			{
				const LilvScalePoint *sp2 = lilv_scale_points_get(sps, iter2);

				if(sp1 == sp2)
				{
					continue; // ignore self
				}

				const LilvNode *val2 = lilv_scale_point_get_value(sp2);
				if(lilv_node_equals(val1, val2))
				{
					ret = &ret_not_unique_val;
					break;
				}

				const LilvNode *lbl2 = lilv_scale_point_get_label(sp2);
				if(lilv_node_equals(lbl1, lbl2))
				{
					ret = &ret_not_unique_lbl;
					break;
				}
			}

			if(ret)
			{
				break;
			}
		}

		lilv_scale_points_free(sps);
	}

	return ret;
}

static const test_t tests [] = {
	{"Port Class",          _test_class},
	{"Port Properties",     _test_properties},
	{"Port Default",        _test_default},
	{"Port Minimum",        _test_minimum},
	{"Port Maximum",        _test_maximum},
	{"Port Range",          _test_range},
	{"Port Event Port",     _test_event_port},
	{"Port Atom Port",      _test_atom_port},
	{"Port Morph Port",     _test_morph_port},
	{"Port Comment",        _test_comment},
	{"Port Group",          _test_group},
	{"Port Units",          _test_unit},
	{"Port Symbol",         _test_symbol},
	{"Port Scale Points",   _test_scale_points},
};

static const unsigned tests_n = sizeof(tests) / sizeof(test_t);

bool
test_port(app_t *app)
{
	bool flag = true;
	bool msg = false;
	res_t *rets = alloca(tests_n * sizeof(res_t));
	if(!rets)
		return flag;

	for(unsigned i=0; i<tests_n; i++)
	{
		const test_t *test = &tests[i];
		res_t *res = &rets[i];

		res->is_whitelisted = lv2lint_test_is_whitelisted(app, app->plugin_uri, test);
		res->urn = NULL;
		app->urn = &res->urn;
		res->ret = test->cb(app);
		const lint_t lnt = lv2lint_extract(app, res->ret);
		if(lnt & app->show)
		{
			msg = true;
		}
	}

	const bool show_passes = LINT_PASS & app->show;

	if(msg || show_passes)
	{
		lv2lint_printf(app, "  %s{%d : %s}%s\n",
			colors[app->atty][ANSI_COLOR_BOLD],
			lilv_port_get_index(app->plugin, app->port),
			lilv_node_as_string(lilv_port_get_symbol(app->plugin, app->port)),
			colors[app->atty][ANSI_COLOR_RESET]);

		for(unsigned i=0; i<tests_n; i++)
		{
			const test_t *test = &tests[i];
			res_t *res = &rets[i];

			lv2lint_report(app, test, res, show_passes, &flag);
		}
	}

	for(unsigned i=0; i<tests_n; i++)
	{
		res_t *res = &rets[i];

		free(res->urn);
	}

	return flag;
}
