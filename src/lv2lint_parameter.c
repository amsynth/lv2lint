/*
 * SPDX-FileCopyrightText: Hanspeter Portner <dev@open-music-kontrollers.ch>
 * SPDX-License-Identifier: Artistic-2.0
 */

#include <lv2lint/lv2lint.h>

#include <lv2/atom/atom.h>
#include <lv2/units/units.h>

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

	LilvNode *label_node = lilv_world_get(app->world, app->parameter, NODE(app, RDFS__label), NULL);
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

	LilvNode *comment_node = lilv_world_get(app->world, app->parameter, NODE(app, RDFS__comment), NULL);
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

	LilvNode *range_node = lilv_world_get(app->world, app->parameter, NODE(app, RDFS__range), NULL);
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
				const LV2_URID ran = app->map->map(app->map->handle, range);

				switch(ran)
				{
					case ATOM__Int:
						// fall-through
					case ATOM__Long:
						// fall-through
					case ATOM__Float:
						// fall-through
					case ATOM__Double:
					{
						LilvNode *minimum = lilv_world_get(app->world, app->parameter, NODE(app, CORE__minimum), NULL);
						if(minimum)
						{
							switch(ran)
							{
								case ATOM__Int:
									// fall-through
								case ATOM__Long:
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
								} break;

								case ATOM__Float:
									// fall-through
								case ATOM__Double:
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
								} break;
							}

							lilv_node_free(minimum);
						}
						else
						{
							ret = &ret_range_minimum_not_found;
						}

						LilvNode *maximum = lilv_world_get(app->world, app->parameter, NODE(app, CORE__maximum), NULL);
						if(maximum)
						{
							switch(ran)
							{
								case ATOM__Int:
									// fall-through
								case ATOM__Long:
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
								} break;

								case ATOM__Float:
									// fall-through
								case ATOM__Double:
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
								} break;
							}

							lilv_node_free(maximum);
						}
						else
						{
							ret = &ret_range_maximum_not_found;
						}

						if(minimum && maximum)
						{
							switch(ran)
							{
								case ATOM__Int:
									// fall-through
								case ATOM__Long:
								{
									if( !(app->min.i64 <= app->max.i64) )
									{
										ret = &ret_range_invalid;
									}
								} break;

								case ATOM__Float:
									// fall-through
								case ATOM__Double:
								{
									if( !(app->min.f64 <= app->max.f64) )
									{
										ret = &ret_range_invalid;
									}
								} break;
							}
						}
					} break;

					case ATOM__Bool:
						// fall-through
					case ATOM__String:
						// fall-through
					case ATOM__Literal:
						// fall-through
					case ATOM__Path:
						// fall-through
					case ATOM__Chunk:
						// fall-through
					case ATOM__URI:
						// fall-through
					case ATOM__URID:
						// fall-through
					case ATOM__Tuple:
						// fall-through
					case ATOM__Object:
						// fall-through
					case ATOM__Vector:
						// fall-through
					case ATOM__Sequence:
						// fall-through
					case XSD__int:
						// fall-through
					case XSD__nonNegativeInteger:
						// fall-through
					case XSD__long:
						// fall-through
					case XSD__float:
						// fall-through
					case XSD__double:
					{
						//OK
					} break;

					default:
					{
						ret = &ret_range_not_an_atom;
					} break;
				}
			}
		}
		else // !is_uri_
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


	LilvNode *unit = lilv_world_get(app->world, app->parameter, NODE(app, UNITS__unit), NULL);
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
	};
	const ret_t *ret = NULL;

	LilvNodes *sps = lilv_world_find_nodes(app->world, app->parameter,
		NODE(app, CORE__scalePoint), NULL);
	if(sps)
	{
		LILV_FOREACH(nodes, iter1, sps)
		{
			const LilvNode *sp1 = lilv_nodes_get(sps, iter1);
			LilvNode *val1 = lilv_world_get(app->world, sp1, NODE(app, RDF__value), NULL);
			LilvNode *lbl1 = lilv_world_get(app->world, sp1, NODE(app, RDFS__label), NULL);

			LILV_FOREACH(nodes, iter2, sps)
			{
				const LilvNode *sp2 = lilv_nodes_get(sps, iter2);

				LilvNode *lbl2 = lilv_world_get(app->world, sp2, NODE(app, RDFS__label), NULL);
				if(lilv_node_equals(lbl1, lbl2))
				{
					lilv_node_free(lbl2);
					continue;
				}
				lilv_node_free(lbl2);

				LilvNode *val2 = lilv_world_get(app->world, sp2, NODE(app, RDF__value), NULL);
				if(lilv_node_equals(val1, val2))
				{
					ret = &ret_not_unique_val;
					lilv_node_free(val2);
					break;
				}
				lilv_node_free(val2);
			}

			lilv_node_free(val1);
			lilv_node_free(lbl1);

			if(ret)
			{
				break;
			}
		}

		lilv_nodes_free(sps);
	}

	return ret;
}

static const test_t tests [] = {
	{"Parameter Label",        _test_label},
	{"Parameter Comment",      _test_comment},
	{"Parameter Range",        _test_range},
	{"Parameter Unit",         _test_unit},
	{"Parameter Scale Points", _test_scale_points},
};

static const unsigned tests_n = sizeof(tests) / sizeof(test_t);

bool
test_parameter(app_t *app)
{
	bool flag = true;
	bool msg = false;
	res_t *rets = alloca(tests_n * sizeof(res_t));
	if(!rets)
	{
		return flag;
	}
	memset(rets, 0x0, tests_n * sizeof(res_t));

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

	for(unsigned i=0; i<tests_n; i++)
	{
		res_t *res = &rets[i];

		free(res->urn);
	}

	return flag;
}
