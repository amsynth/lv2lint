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

#include <lv2lint.h>

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/extensions/units/units.h>

static const ret_t *
_test_label(app_t *app)
{
	static const ret_t ret_label_not_found = {
		.lnt = LINT_FAIL,
		.msg = "rdfs:label not found",
		.uri = LV2_CORE_PREFIX"Parameter",
		.dsc = NULL
	},
	ret_label_not_a_string = {
		.lnt = LINT_FAIL,
		.msg = "rdfs:label not a string",
		.uri = LILV_NS_DOAP"label",
		.dsc = NULL
	},
	ret_label_empty = {
		.lnt = LINT_FAIL,
		.msg = "rdfs:label empty",
		.uri = LILV_NS_DOAP"label",
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	LilvNode *label_node = lilv_world_get(app->world, app->parameter, app->uris.rdfs_label, NULL);
	if(label_node)
	{
		if(lilv_node_is_string(label_node))
		{
			const char *label = lilv_node_as_string(label_node);
			if(!label)
			{
				ret = &ret_label_empty;
			}
		}
		else // !is_string
		{
			ret = &ret_label_not_a_string;
		}
		lilv_node_free(label_node);
	}
	else // !label_node
	{
		ret = &ret_label_not_found;
	}

	return ret;
}


static const ret_t *
_test_comment(app_t *app)
{
	static const ret_t ret_comment_not_found = {
		.lnt = LINT_NOTE,
		.msg = "rdfs:comment not found",
		.uri = LV2_CORE_PREFIX"Parameter",
		.dsc = "Adding comment helps the user to understand this parameter."
	},
	ret_comment_not_a_string = {
		.lnt = LINT_FAIL,
		.msg = "rdfs:comment not a string",
		.uri = LILV_NS_DOAP"comment",
		.dsc = NULL
	},
	ret_comment_empty = {
		.lnt = LINT_FAIL,
		.msg = "rdfs:comment empty",
		.uri = LILV_NS_DOAP"comment",
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	LilvNode *comment_node = lilv_world_get(app->world, app->parameter, app->uris.rdfs_comment, NULL);
	if(comment_node)
	{
		if(lilv_node_is_string(comment_node))
		{
			const char *comment = lilv_node_as_string(comment_node);
			if(!comment)
			{
				ret = &ret_comment_empty;
			}
		}
		else // !is_string
		{
			ret = &ret_comment_not_a_string;
		}
		lilv_node_free(comment_node);
	}
	else // !comment_node
	{
		ret = &ret_comment_not_found;
	}

	return ret;
}

static const ret_t *
_test_range(app_t *app)
{
	static const ret_t ret_range_not_found = {
		.lnt = LINT_FAIL,
		.msg = "rdfs:range not found",
		.uri = LV2_CORE_PREFIX"Parameter",
		.dsc = NULL
	},
	ret_range_not_a_uri = {
		.lnt = LINT_FAIL,
		.msg = "rdfs:range not a URI",
		.uri = LILV_NS_DOAP"range",
		.dsc = NULL
	},
	ret_range_not_an_atom = {
		.lnt = LINT_WARN,
		.msg = "rdfs:range not an lv2:Atom",
		.uri = LV2_ATOM__Atom,
		.dsc = NULL
	},
	ret_range_empty = {
		.lnt = LINT_FAIL,
		.msg = "rdfs:range empty",
		.uri = LILV_NS_DOAP"range",
		.dsc = NULL
	},
	ret_range_minimum_not_found = {
		.lnt = LINT_WARN,
		.msg = "lv2:minimum not found",
		.uri = LV2_CORE__minimum,
		.dsc = NULL
	},
	ret_range_minimum_not_an_int = {
		.lnt = LINT_FAIL,
		.msg = "lv2:minimum not an integer",
		.uri = LV2_CORE__minimum,
		.dsc = NULL
	},
	ret_range_minimum_not_a_float = {
		.lnt = LINT_FAIL,
		.msg = "lv2:minimum not a float",
		.uri = LV2_CORE__minimum,
		.dsc = NULL
	},
	ret_range_maximum_not_found = {
		.lnt = LINT_WARN,
		.msg = "lv2:maximum not found",
		.uri = LV2_CORE__maximum,
		.dsc = NULL
	},
	ret_range_maximum_not_an_int = {
		.lnt = LINT_FAIL,
		.msg = "lv2:maximum not an integer",
		.uri = LV2_CORE__maximum,
		.dsc = NULL
	},
	ret_range_maximum_not_a_float = {
		.lnt = LINT_FAIL,
		.msg = "lv2:maximum not a float",
		.uri = LV2_CORE__maximum,
		.dsc = NULL
	},
	ret_range_invalid = {
		.lnt = LINT_FAIL,
		.msg = "range invalid (min <= max)",
		.uri = LV2_CORE_PREFIX"Parameter",
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	LilvNode *range_node = lilv_world_get(app->world, app->parameter, app->uris.rdfs_range, NULL);
	if(range_node)
	{
		if(lilv_node_is_uri(range_node))
		{
			const char *range = lilv_node_as_uri(range_node);
			if(!range)
			{
				ret = &ret_range_empty;
			}
			else
			{
				if(  lilv_node_equals(range_node, app->uris.atom_Int)
					|| lilv_node_equals(range_node, app->uris.atom_Long)
					|| lilv_node_equals(range_node, app->uris.atom_Float)
					|| lilv_node_equals(range_node, app->uris.atom_Double) )
				{
					LilvNode *minimum = lilv_world_get(app->world, app->parameter, app->uris.lv2_minimum, NULL);
					if(minimum)
					{
						if(  lilv_node_equals(range_node, app->uris.atom_Int)
							|| lilv_node_equals(range_node, app->uris.atom_Long) )
						{
							if(!lilv_node_is_int(minimum))
							{
								ret = &ret_range_minimum_not_an_int;
								app->min.i64 = 0; // fall-back
							}
							else
							{
								app->min.i64 = lilv_node_as_int(minimum);
							}
						}
						else if(lilv_node_equals(range_node, app->uris.atom_Float)
							|| lilv_node_equals(range_node, app->uris.atom_Double) )
						{
							if(!lilv_node_is_float(minimum))
							{
								ret = &ret_range_minimum_not_a_float;
								app->min.f64 = 0.0; // fall-back
							}
							else
							{
								app->min.f64 = lilv_node_as_float(minimum);
							}
						}

						lilv_node_free(minimum);
					}
					else
					{
						ret = &ret_range_minimum_not_found;
					}

					LilvNode *maximum = lilv_world_get(app->world, app->parameter, app->uris.lv2_maximum, NULL);
					if(maximum)
					{
						if(  lilv_node_equals(range_node, app->uris.atom_Int)
							|| lilv_node_equals(range_node, app->uris.atom_Long) )
						{
							if(!lilv_node_is_int(maximum))
							{
								ret = &ret_range_maximum_not_an_int;
								app->max.i64 = 1; // fall-back
							}
							else
							{
								app->max.i64 = lilv_node_as_int(maximum);
							}
						}
						else if(lilv_node_equals(range_node, app->uris.atom_Float)
							|| lilv_node_equals(range_node, app->uris.atom_Double) )
						{
							if(!lilv_node_is_float(maximum))
							{
								ret = &ret_range_maximum_not_a_float;
								app->max.f64 = 1.0; // fall-back
							}
							else
							{
								app->max.f64 = lilv_node_as_float(maximum);
							}
						}

						lilv_node_free(maximum);
					}
					else
					{
						ret = &ret_range_maximum_not_found;
					}

					if(minimum && maximum)
					{
						if(  lilv_node_equals(range_node, app->uris.atom_Int)
							|| lilv_node_equals(range_node, app->uris.atom_Long) )
						{
							if( !(app->min.i64 <= app->max.i64) )
							{
								ret = &ret_range_invalid;
							}
						}
						else if(lilv_node_equals(range_node, app->uris.atom_Float)
							|| lilv_node_equals(range_node, app->uris.atom_Double) )
						{
							if( !(app->min.f64 <= app->max.f64) )
							{
								ret = &ret_range_invalid;
							}
						}
					}
				}
				else if(lilv_node_equals(range_node, app->uris.atom_Bool)
					|| lilv_node_equals(range_node, app->uris.atom_String)
					|| lilv_node_equals(range_node, app->uris.atom_Literal)
					|| lilv_node_equals(range_node, app->uris.atom_Path)
					|| lilv_node_equals(range_node, app->uris.atom_Chunk)
					|| lilv_node_equals(range_node, app->uris.atom_URI)
					|| lilv_node_equals(range_node, app->uris.atom_URID)
					|| lilv_node_equals(range_node, app->uris.atom_Tuple)
					|| lilv_node_equals(range_node, app->uris.atom_Object)
					|| lilv_node_equals(range_node, app->uris.atom_Vector)
					|| lilv_node_equals(range_node, app->uris.atom_Sequence)
					|| lilv_node_equals(range_node, app->uris.xsd_int)
					|| lilv_node_equals(range_node, app->uris.xsd_uint)
					|| lilv_node_equals(range_node, app->uris.xsd_long)
					|| lilv_node_equals(range_node, app->uris.xsd_float)
					|| lilv_node_equals(range_node, app->uris.xsd_double) )
				{
					// OK
				}
				else
				{
					ret = &ret_range_not_an_atom;
				}
			}
		}
		else // !is_string
		{
			ret = &ret_range_not_a_uri;
		}
		lilv_node_free(range_node);
	}
	else // !range_node
	{
		ret = &ret_range_not_found;
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
		.dsc = "Adding units to paramters helps the user to put things in perspective."
	},
	ret_units_unit_not_a_uri_or_object = {
		.lnt = LINT_FAIL,
		.msg = "units_unit not a URI or object",
		.uri = LV2_UNITS__unit,
		.dsc = NULL
	};

	const ret_t *ret = NULL;


	LilvNode *unit = lilv_world_get(app->world, app->parameter, app->uris.units_unit, NULL);
	if(unit)
	{
		if(  !lilv_node_is_uri(unit)
			&& !lilv_world_ask(app->world, unit, app->uris.rdf_type, app->uris.units_Unit) )
		{
			ret = &ret_units_unit_not_a_uri_or_object;
		}

		lilv_node_free(unit);
	}
	else
	{
		ret = &ret_units_unit_not_found;
	}

	return ret;
}

static const test_t tests [] = {
	{"Parameter Label",   _test_label},
	{"Parameter Comment", _test_comment},
	{"Parameter Range",   _test_range},
	{"Parameter Unit",    _test_unit},
	//TODO scalePoint
};

static const unsigned tests_n = sizeof(tests) / sizeof(test_t);

bool
test_parameter(app_t *app)
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
		lv2lint_printf(app, "  %s<%s>%s\n",
			colors[app->atty][ANSI_COLOR_BOLD],
			lilv_node_as_uri(app->parameter),
			colors[app->atty][ANSI_COLOR_RESET]);

		for(unsigned i=0; i<tests_n; i++)
		{
			const test_t *test = &tests[i];
			res_t *res = &rets[i];

			lv2lint_report(app, test, res, show_passes, &flag);
		}
	}

	return flag;
}
