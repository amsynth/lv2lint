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

#include <lv2lint.h>

#include <lv2/patch/patch.h>
#include <lv2/worker/worker.h>
#include <lv2/uri-map/uri-map.h>
#include <lv2/state/state.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/ui/ui.h>
#include <lv2/ui/ui.h>

static const ret_t *
_test_lv2_path(app_t *app)
{
	static const ret_t ret_no_ui_class = {
		.lnt = LINT_FAIL,
		.msg = "failed to find ui:X11UI's label with LV2_PATH='%s'",
		.uri = LV2_UI__X11UI,
		.dsc = "You likely have a borked LV2_PATH, make sure the whole LV2 spec\n"
			"is part of your LV2_PATH."
	},
	ret_no_plugin_class = {
		.lnt = LINT_FAIL,
		.msg = "failed to find core:InstrumentPlugin's label with LV2_PATH='%s'",
		.uri = LV2_CORE__InstrumentPlugin,
		.dsc = "You likely have a borked LV2_PATH, make sure the whole LV2 spec\n"
			"is part of your LV2_PATH."
	};

	const ret_t *ret = NULL;

	LilvNode *ui_class_node = lilv_world_get(app->world,
		NODE(app, UI__X11UI), NODE(app, RDFS__label), NULL);
	LilvNode *plugin_class_node = lilv_world_get(app->world,
		NODE(app, CORE__InstrumentPlugin), NODE(app, RDFS__label), NULL);

	const char *lv2_path = getenv("LV2_PATH");

	if(!ui_class_node)
	{
		*app->urn = lv2lint_strdup(lv2_path);
		ret = &ret_no_ui_class;
	}
	else if(!plugin_class_node)
	{
		*app->urn = lv2lint_strdup(lv2_path);
		ret = &ret_no_plugin_class;
	}

	if(ui_class_node)
	{
		lilv_node_free(ui_class_node);
	}
	if(plugin_class_node)
	{
		lilv_node_free(plugin_class_node);
	}

	return ret;
}

static const ret_t *
_test_instantiation(app_t *app)
{
	static const ret_t ret_instantiation = {
		.lnt = LINT_FAIL,
		.msg = "failed to instantiate",
		.uri = LV2_CORE_URI,
		.dsc = "You likely have forgotten to list all lv2:requiredFeature's."
	};

	const ret_t *ret = NULL;

	if(!app->instance)
	{
		ret = &ret_instantiation;
	}

	return ret;
}

#ifdef ENABLE_ELF_TESTS
static const ret_t *
_test_symbols(app_t *app)
{
	static const ret_t ret_symbols = {
		.lnt = LINT_FAIL,
		.msg = "binary exports superfluous globally visible symbols: %s",
		.uri = LV2_CORE__binary,
		.dsc = "Plugin binaries must not export any globally visible symbols "
			 "but lv2_descriptor. You may well have forgotten to compile "
			 "with -fvisibility=hidden."
	};

	const ret_t *ret = NULL;

	const LilvNode* node = lilv_plugin_get_library_uri(app->plugin);
	if(node && lilv_node_is_uri(node))
	{
		const char *uri = lilv_node_as_uri(node);
		if(uri)
		{
			char *path = lilv_file_uri_parse(uri, NULL);
			if(path)
			{
				char *symbols = NULL;
				if(!test_visibility(app, path, app->plugin_uri, "lv2_descriptor", &symbols))
				{
					*app->urn = symbols;
					ret = &ret_symbols;
				}
				else if(symbols)
				{
					free(symbols);
				}

				lilv_free(path);
			}
		}
	}

	return ret;
}

static const ret_t *
_test_fork(app_t *app)
{
	static const ret_t ret_fork = {
		.lnt = LINT_WARN,
		.msg = "binary has a symbol reference to the 'fork' function",
		.uri = LV2_CORE__binary,
		.dsc = "Plugin binaries must not call 'fork', as it may interrupt "
			"the whole realtime plugin graph and lead to unwanted xruns."
	};

	const ret_t *ret = NULL;

	const LilvNode* node = lilv_plugin_get_library_uri(app->plugin);
	if(node && lilv_node_is_uri(node))
	{
		const char *uri = lilv_node_as_uri(node);
		if(uri)
		{
			char *path = lilv_file_uri_parse(uri, NULL);
			if(path)
			{
				if(check_for_symbol(app, path, "fork"))
				{
					ret = &ret_fork;
				}

				lilv_free(path);
			}
		}
	}

	return ret;
}

static const ret_t *
_test_linking(app_t *app)
{
	static const ret_t ret_symbols = {
		.lnt = LINT_WARN,
		.pck = LINT_NOTE,
		.msg = "binary links to non-whitelisted shared libraries: %s",
		.uri = LV2_CORE__binary,
		.dsc = "The ideal plugin dynamically links maximally to libc, libm, librt, "
			"libstdc++, libgcc_s."
	},
	ret_libstdcpp = {
		.lnt = LINT_WARN,
		.pck = LINT_NOTE,
		.msg = "binary links to C++ libraries: %s",
		.uri = LV2_CORE__binary,
		.dsc = "C++ ABI incompatibilities between host and plugin are to be expected."
	};

	const ret_t *ret = NULL;

	static const char *whitelist [] = {
		"libc",
		"libm",
		"librt",
		"libstdc++",
		"libgcc_s"
	};
	const unsigned n_whitelist = sizeof(whitelist) / sizeof(const char *);

	static const char *graylist [] = {
		"libstdc++",
		"libgcc_s"
	};
	const unsigned n_graylist = sizeof(graylist) / sizeof(const char *);

	const LilvNode* node = lilv_plugin_get_library_uri(app->plugin);
	if(node && lilv_node_is_uri(node))
	{
		const char *uri = lilv_node_as_uri(node);
		if(uri)
		{
			char *path = lilv_file_uri_parse(uri, NULL);
			if(path)
			{
				char *libraries = NULL;
				if(!test_shared_libraries(app, path, app->plugin_uri,
					whitelist, n_whitelist,
					NULL, 0,
					&libraries))
				{
					*app->urn = libraries;
					ret = &ret_symbols;
				}
				else if(!test_shared_libraries(app, path, app->plugin_uri,
					NULL, 0,
					graylist, n_graylist,
					&libraries))
				{
					*app->urn = libraries;
					ret = &ret_libstdcpp;
				}
				else if(libraries)
				{
					free(libraries);
				}

				lilv_free(path);
			}
		}
	}

	return ret;
}
#endif

static const ret_t *
_test_verification(app_t *app)
{
	static const ret_t ret_verification = {
		.lnt = LINT_FAIL,
		.msg = "failed lilv_plugin_verify",
		.uri = LV2_CORE_URI,
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	if(!lilv_plugin_verify(app->plugin))
	{
		ret = &ret_verification;
	}

	return ret;
}

static const ret_t *
_test_name(app_t *app)
{
	static const ret_t ret_name_not_found = {
		.lnt = LINT_FAIL,
		.msg = "doap:name not found",
		.uri = LV2_CORE__Plugin,
		.dsc = NULL
	},
	ret_name_not_a_string = {
		.lnt = LINT_FAIL,
		.msg = "doap:name not a string",
		.uri = LILV_NS_DOAP"name",
		.dsc = NULL
	},
	ret_name_empty = {
		.lnt = LINT_FAIL,
		.msg = "doap:name empty",
		.uri = LILV_NS_DOAP"name",
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	LilvNode *name_node = lilv_plugin_get_name(app->plugin);
	if(name_node)
	{
		if(lilv_node_is_string(name_node))
		{
			const char *name = lilv_node_as_string(name_node);
			if(!name)
			{
				ret = &ret_name_empty;
			}
		}
		else // !is_string
		{
			ret = &ret_name_not_a_string;
		}
		lilv_node_free(name_node);
	}
	else // !name_node
	{
		ret = &ret_name_not_found;
	}

	return ret;
}

static const ret_t *
_test_license(app_t *app)
{
	static const ret_t ret_license_not_found = {
		.lnt = LINT_WARN,
		.msg = "doap:license not found",
		.uri = LV2_CORE__Plugin,
		.dsc = NULL
	},
	ret_license_not_a_uri = {
		.lnt = LINT_FAIL,
		.msg = "doap:license not a URI",
		.uri = LILV_NS_DOAP"license",
		.dsc = NULL
	},
#ifdef ENABLE_ONLINE_TESTS
	ret_license_not_existing = {
		.lnt = LINT_WARN,
		.msg = "doap:license Web URL does not exist",
		.uri = LILV_NS_DOAP"license",
		.dsc = "It is good practice to have some online documentation at given URL."
	},
#endif
	ret_license_empty = {
		.lnt = LINT_FAIL,
		.msg = "doap:license empty",
		.uri = LILV_NS_DOAP"license",
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	LilvNode *license_node = lilv_world_get(app->world, lilv_plugin_get_uri(app->plugin), NODE(app, DOAP__license), NULL);
	if (!license_node)
	{
		LilvNode *project_node = lilv_plugin_get_project(app->plugin);
		if (project_node)
		{
			license_node = lilv_world_get(app->world, project_node, NODE(app, DOAP__license), NULL);
		}
	}

	if(license_node)
	{
		if(lilv_node_is_uri(license_node))
		{
			const char *uri = lilv_node_as_uri(license_node);

			if(!uri)
			{
				ret = &ret_license_empty;
			}
#ifdef ENABLE_ONLINE_TESTS
			else if(is_url(uri))
			{
				const bool url_exists = !app->online || test_url(app, uri);

				if(!url_exists)
				{
					ret = &ret_license_not_existing;
				}
			}
#endif
		}
		else
		{
			ret = &ret_license_not_a_uri;
		}
		lilv_node_free(license_node);
	}
	else
	{
		ret = &ret_license_not_found;
	}

	return ret;
}

#define FOAF_DSC "You likely have not defined an lv2:project with " \
	"a valid doap:maintainer or your plugin is not a subclass of doap:Project."

static const ret_t *
_test_author_name(app_t *app)
{
	static const ret_t ret_author_not_found = {
		.lnt = LINT_WARN,
		.msg = "foaf:name not found",
		.uri = LV2_CORE__project,
		.dsc = FOAF_DSC
	},
	ret_author_not_a_string = {
		.lnt = LINT_FAIL,
		.msg = "foaf:name not a string",
		.uri = LILV_NS_FOAF"name",
		.dsc = FOAF_DSC
	},
	ret_author_empty = {
		.lnt = LINT_FAIL,
		.msg = "foaf:name empty",
		.uri = LILV_NS_FOAF"name",
		.dsc = FOAF_DSC
	};

	const ret_t *ret = NULL;

	LilvNode *author_name = lilv_plugin_get_author_name(app->plugin);
	if(author_name)
	{
		if(lilv_node_is_string(author_name))
		{
			if(!lilv_node_as_string(author_name))
			{
				ret = &ret_author_empty;
			}
		}
		else
		{
			ret = &ret_author_not_a_string;
		}
		lilv_node_free(author_name);
	}
	else
	{
		ret = &ret_author_not_found;
	}

	return ret;
}

static const ret_t *
_test_author_email(app_t *app)
{
	static const ret_t ret_email_not_found = {
		.lnt = LINT_WARN,
		.msg = "foaf:mbox not found",
		.uri = LV2_CORE__project,
		.dsc = FOAF_DSC
	},
	ret_email_not_a_uri = {
		.lnt = LINT_FAIL,
		.msg = "foaf:mbox not a URI",
		.uri = LILV_NS_FOAF"mbox",
		.dsc = FOAF_DSC
	},
	ret_email_empty = {
		.lnt = LINT_FAIL,
		.msg = "foaf:mbox empty",
		.uri = LILV_NS_FOAF"mbox",
		.dsc = FOAF_DSC
	};

	const ret_t *ret = NULL;

	LilvNode *author_email = lilv_plugin_get_author_email(app->plugin);
	if(author_email)
	{
		if(lilv_node_is_uri(author_email))
		{
			if(!lilv_node_as_uri(author_email))
			{
				ret = &ret_email_empty;
			}
		}
		else
		{
			ret = &ret_email_not_a_uri;
		}
		lilv_node_free(author_email);
	}
	else
	{
		ret = &ret_email_not_found;
	}

	return ret;
}

static const ret_t *
_test_author_homepage(app_t *app)
{
	static const ret_t ret_homepage_not_found = {
		.lnt = LINT_WARN,
		.msg = "foaf:homepage not found",
		.uri = LV2_CORE__project,
		.dsc = FOAF_DSC
	},
	ret_homepage_not_a_uri = {
		.lnt = LINT_FAIL,
		.msg = "foaf:homepage not a URI",
		.uri = LILV_NS_FOAF"homepage",
		.dsc = FOAF_DSC
	},
#ifdef ENABLE_ONLINE_TESTS
	ret_homepage_not_existing = {
		.lnt = LINT_WARN,
		.msg = "foaf:homepage Web URL does not exist",
		.uri = LILV_NS_FOAF"homepage",
		.dsc = FOAF_DSC
	},
#endif
	ret_homepage_empty = {
		.lnt = LINT_FAIL,
		.msg = "foaf:homepage empty",
		.uri = LILV_NS_FOAF"homepage",
		.dsc = FOAF_DSC
	};

	const ret_t *ret = NULL;

	LilvNode *author_homepage = lilv_plugin_get_author_homepage(app->plugin);
	if(author_homepage)
	{
		if(lilv_node_is_uri(author_homepage))
		{
			const char *uri = lilv_node_as_uri(author_homepage);

			if(!uri)
			{
				ret = &ret_homepage_empty;
			}
#ifdef ENABLE_ONLINE_TESTS
			else if(is_url(uri))
			{
				const bool url_exists = !app->online || test_url(app, uri);

				if(!url_exists)
				{
					ret = &ret_homepage_not_existing;
				}
			}
#endif
		}
		else
		{
			ret = &ret_homepage_not_a_uri;
		}
		lilv_node_free(author_homepage);
	}
	else
	{
		ret = &ret_homepage_not_found;
	}

	return ret;
}

static const ret_t *
_test_version_minor(app_t *app)
{
	static const ret_t ret_version_minor_not_found = {
		.lnt = LINT_FAIL,
		.msg = "lv2:minorVersion not found",
		.uri = LV2_CORE__minorVersion,
		.dsc = NULL
	},
	ret_version_minor_not_an_int = {
		.lnt = LINT_FAIL,
		.msg = "lv2:minorVersion not an integer",
		.uri = LV2_CORE__minorVersion,
		.dsc = NULL
	},
	ret_version_minor_unstable = {
		.lnt = LINT_NOTE,
		.msg = "lv2:minorVersion denotes an unstable version",
		.uri = LV2_CORE__minorVersion,
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	LilvNodes *minor_version_nodes = lilv_plugin_get_value(app->plugin , NODE(app, CORE__minorVersion));
	if(minor_version_nodes)
	{
		const LilvNode *minor_version_node = lilv_nodes_get_first(minor_version_nodes);
		if(minor_version_node)
		{
			if(lilv_node_is_int(minor_version_node))
			{
				const int minor_version = lilv_node_as_int(minor_version_node);
				if( (minor_version % 2 != 0) || (minor_version == 0) )
				{
					ret = &ret_version_minor_unstable;
				}
			}
			else
			{
				ret = &ret_version_minor_not_an_int;
			}
		}
		else
		{
			ret = &ret_version_minor_not_found;
		}
		lilv_nodes_free(minor_version_nodes);
	}
	else
	{
		ret = &ret_version_minor_not_found;
	}

	return ret;
}

static const ret_t *
_test_version_micro(app_t *app)
{
	static const ret_t ret_version_micro_not_found = {
		.lnt = LINT_FAIL,
		.msg = "lv2:microVersion not found",
		.uri = LV2_CORE__microVersion,
		.dsc = NULL
	},
	ret_version_micro_not_an_int = {
		.lnt = LINT_FAIL,
		.msg = "lv2:microVersion not an integer",
		.uri = LV2_CORE__microVersion,
		.dsc = NULL
	},
	ret_version_micro_unstable = {
		.lnt = LINT_NOTE,
		.msg = "lv2:microVersion denotes an unstable version",
		.uri = LV2_CORE__microVersion,
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	LilvNodes *micro_version_nodes = lilv_plugin_get_value(app->plugin , NODE(app, CORE__microVersion));
	if(micro_version_nodes)
	{
		const LilvNode *micro_version_node = lilv_nodes_get_first(micro_version_nodes);
		if(micro_version_node)
		{
			if(lilv_node_is_int(micro_version_node))
			{
				const int micro_version = lilv_node_as_int(micro_version_node);
				if(micro_version % 2 != 0)
				{
					ret = &ret_version_micro_unstable;
				}
			}
			else
			{
				ret = &ret_version_micro_not_an_int;
			}
		}
		else
		{
			ret = &ret_version_micro_not_found;
		}
		lilv_nodes_free(micro_version_nodes);
	}
	else
	{
		ret = &ret_version_micro_not_found;
	}

	return ret;
}

static const ret_t *
_test_project(app_t *app)
{
	static const ret_t ret_project_not_found = {
		.lnt = LINT_NOTE,
		.msg = "lv2:project not found",
		.uri = LV2_CORE__project,
		.dsc = NULL
	},
	ret_project_name_not_found = {
		.lnt = LINT_WARN,
		.msg = "lv2:project doap:name not found",
		.uri = LV2_CORE__project,
		.dsc = NULL
	},
	ret_project_name_not_a_string = {
		.lnt = LINT_FAIL,
		.msg = "lv2:project doap:name not a string",
		.uri = LILV_NS_DOAP"name",
		.dsc = NULL
	},
	ret_project_name_empty = {
		.lnt = LINT_FAIL,
		.msg = "lv2:project doap:name empty",
		.uri = LILV_NS_DOAP"name",
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	LilvNode *project_node = lilv_plugin_get_project(app->plugin);
	if(project_node)
	{
		LilvNode *project_name_node = lilv_world_get(app->world, project_node, NODE(app, DOAP__name), NULL);
		if(project_name_node)
		{
			if(lilv_node_is_string(project_name_node))
			{
				if(!lilv_node_as_string(project_name_node))
				{
					ret = &ret_project_name_empty;
				}
			}
			else
			{
				ret = &ret_project_name_not_a_string;
			}
			lilv_free(project_name_node);
		}
		else // !doap_name_node
		{
			ret = &ret_project_name_not_found;
		}
		lilv_node_free(project_node);
	}
	else // !project_node
	{
		ret = &ret_project_not_found;
	}

	return ret;
}

static inline bool
_test_class_equals(const LilvPluginClass *base, const LilvPluginClass *class)
{
	return lilv_node_equals(
		lilv_plugin_class_get_uri(base),
		lilv_plugin_class_get_uri(class) );
}

static inline bool
_test_class_match(const LilvPluginClass *base, const LilvPluginClass *class)
{
	if(_test_class_equals(base, class))
	{
		return true;
	}

	bool ret = false;
	LilvPluginClasses *children= lilv_plugin_class_get_children(base);
	if(children)
	{
		LILV_FOREACH(plugin_classes, itr, children)
		{
			const LilvPluginClass *child = lilv_plugin_classes_get(children, itr);
			if(_test_class_match(child, class))
			{
				ret = true;
				break;
			}
		}
		lilv_plugin_classes_free(children);
	}

	return ret;
}

static const ret_t *
_test_class(app_t *app)
{
	static const ret_t ret_class_not_found = {
		.lnt = LINT_FAIL,
		.msg = "type not found",
		.uri = LV2_CORE__Plugin,
		.dsc = NULL
	},
	ret_class_is_base_class = {
		.lnt = LINT_WARN,
		.msg = "type is just lv2:Plugin",
		.uri = LV2_CORE__Plugin,
		.dsc = "If you give the plugin a proper class, hosts can better sort them."
	},
	ret_class_not_valid = {
		.lnt = LINT_FAIL,
		.msg = "type <%s> not valid",
		.uri = LV2_CORE__Plugin,
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	const LilvPluginClass *class = lilv_plugin_get_class(app->plugin);
	if(class)
	{
		const LilvPluginClass *base = lilv_world_get_plugin_class(app->world);
		if(_test_class_equals(base, class))
		{
			ret = &ret_class_is_base_class;
		}
		else if(!_test_class_match(base, class))
		{
			const LilvNode *class_uri = lilv_plugin_class_get_uri(class);
			*app->urn = lv2lint_node_as_uri_strdup(class_uri);
			ret = &ret_class_not_valid;
		}
	}
	else // !class
	{
		ret = &ret_class_not_found;
	}

	return ret;
}

static const ret_t *
_test_features(app_t *app)
{
	static const ret_t ret_features_not_valid = {
		.lnt = LINT_FAIL,
		.msg = "lv2:[optional|required]Feature <%s> not valid",
		.uri = LV2_CORE__Feature,
		.dsc = "Make sure that the lv2:Feature is defined somewhere."
	};

	const ret_t *ret = NULL;

	LilvNodes *features = lilv_world_find_nodes(app->world,
		NULL, NODE(app, RDF__type), NODE(app, CORE__Feature));
	if(features)
	{
		LilvNodes *supported = lilv_plugin_get_supported_features(app->plugin);
		if(supported)
		{
			LILV_FOREACH(nodes, itr, supported)
			{
				const LilvNode *node = lilv_nodes_get(supported, itr);

				if(!lilv_nodes_contains(features, node))
				{
					*app->urn = lv2lint_node_as_uri_strdup(node);
					ret = &ret_features_not_valid;
					break;
				}
			}

			lilv_nodes_free(supported);
		}

		lilv_nodes_free(features);
	}

	return ret;
}

static const ret_t *
_test_extensions(app_t *app)
{
	static const ret_t ret_extensions_not_valid = {
		.lnt = LINT_FAIL,
		.msg = "lv2:extensionData <%s> not valid",
		.uri = LV2_CORE__ExtensionData,
		.dsc = "Make sure that the lv2:extensionData is defined somewhere."
	},
	ret_extensions_data_not_valid = {
		.lnt = LINT_FAIL,
		.msg = "extension data for <%s> not returned",
		.uri = LV2_CORE__ExtensionData,
		.dsc = "You likely do not properly check the URI in your plugin's "
			"'extension_data' callback or don't have the latter at all."
	},
	ret_extensions_data_not_null = {
		.lnt = LINT_FAIL,
		.msg = "extension data for <%s> not NULL",
		.uri = LV2_CORE__ExtensionData,
		.dsc = "You likely do not properly check the URI in your plugin's "
			"'extension_data' callback or don't have the latter at all."
	};

	const ret_t *ret = NULL;

	if(app->instance)
	{
		const char *uri = "http://open-music-kontrollers.ch/lv2/lv2lint#dummy";
		const void *ext = lilv_instance_get_extension_data(app->instance, uri);
		if(ext)
		{
			*app->urn = lv2lint_strdup(uri);
			ret = &ret_extensions_data_not_null;
		}
	}

	LilvNodes *extensions = lilv_world_find_nodes(app->world,
		NULL, NODE(app, RDF__type), NODE(app, CORE__ExtensionData));
	if(extensions)
	{
		LilvNodes *data = lilv_plugin_get_extension_data(app->plugin);
		if(data)
		{
			LILV_FOREACH(nodes, itr, data)
			{
				const LilvNode *node = lilv_nodes_get(data, itr);

				if(!lilv_nodes_contains(extensions, node))
				{
					*app->urn = lv2lint_node_as_uri_strdup(node);
					ret = &ret_extensions_not_valid;
					break;
				}

				if(app->instance)
				{
					const char *uri = lilv_node_as_uri(node);
					const void *ext = lilv_instance_get_extension_data(app->instance, uri);
					if(!ext)
					{
						*app->urn = lv2lint_node_as_uri_strdup(node);
						ret = &ret_extensions_data_not_valid;
						break;
					}
				}
			}

			lilv_nodes_free(data);
		}

		lilv_nodes_free(extensions);
	}

	return ret;
}

static const ret_t *
_test_worker(app_t *app)
{
	static const ret_t ret_worker_schedule_not_found = {
		.lnt = LINT_FAIL,
		.msg = "work:schedule not defined",
		.uri = LV2_WORKER__schedule,
		.dsc = "The plugin exposes the worker extension, but does not list this "
			"lv2:Feature."
	},
	ret_worker_interface_not_found = {
		.lnt = LINT_FAIL,
		.msg = "work:interface not defined",
		.uri = LV2_WORKER__interface,
		.dsc = "The plugin exposes the worker extension, but does not list this "
			"lv2:ExtensionData."
	},
	ret_worker_interface_not_returned = {
		.lnt = LINT_FAIL,
		.msg = "work:interface not returned by 'extention_data'",
		.uri = LV2_WORKER__interface,
		.dsc = "The plugin returns no struct in 'extension_data' callback."
	},
	ret_worker_work_not_found = {
		.lnt = LINT_FAIL,
		.msg = "work:interface has no 'work' function",
		.uri = LV2_WORKER__interface,
		.dsc = NULL
	},
	ret_worker_work_response_not_found = {
		.lnt = LINT_FAIL,
		.msg = "work:interface has no 'work_response' function",
		.uri = LV2_WORKER__interface,
		.dsc = NULL
	},
	ret_worker_end_run_not_found = {
		.lnt = LINT_NOTE,
		.msg = "work:interface has no 'end_run' function",
		.uri = LV2_WORKER__interface,
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	const bool has_work_schedule= lilv_plugin_has_feature(app->plugin, NODE(app, WORKER__schedule));
	const bool has_work_iface = lilv_plugin_has_extension_data(app->plugin, NODE(app, WORKER__interface));

	if(has_work_schedule || has_work_iface || app->work_iface)
	{
		if(!app->work_iface)
		{
			ret = &ret_worker_interface_not_returned;
		}
		else if(!app->work_iface->work)
		{
			ret = &ret_worker_work_not_found;
		}
		else if(!app->work_iface->work_response)
		{
			ret = &ret_worker_work_response_not_found;
		}
		else if(!app->work_iface->end_run)
		{
			ret = &ret_worker_end_run_not_found;
		}
		else if(!has_work_schedule)
		{
			ret = &ret_worker_schedule_not_found;
		}
		else if(!has_work_iface)
		{
			ret = &ret_worker_interface_not_found;
		}
	}

	return ret;
}

static const ret_t *
_test_options_iface(app_t *app)
{
	static const ret_t ret_options_interface_not_found = {
		.lnt = LINT_FAIL,
		.msg = "opts:interface not defined",
		.uri = LV2_OPTIONS__interface,
		.dsc = "The plugin exposes the options extension, but does not list this "
			"lv2:Feature."
	},
	ret_options_interface_not_returned = {
		.lnt = LINT_FAIL,
		.msg = "opts:interface not returned by 'extention_data'",
		.uri = LV2_OPTIONS__interface,
		.dsc = "The plugin returns no struct in 'extension_data' callback."
	},
	ret_options_get_not_found = {
		.lnt = LINT_FAIL,
		.msg = "opts:interface has no 'get' function",
		.uri = LV2_OPTIONS__interface,
		.dsc = NULL
	},
	ret_options_set_not_found = {
		.lnt = LINT_FAIL,
		.msg = "opts:interface has no 'set' function",
		.uri = LV2_OPTIONS__interface,
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	const bool has_opts_iface = lilv_plugin_has_extension_data(app->plugin, NODE(app, OPTIONS__interface));

	if(has_opts_iface || app->opts_iface)
	{
		if(!app->opts_iface)
		{
			ret = &ret_options_interface_not_returned;
		}
		else if(!app->opts_iface->get)
		{
			ret = &ret_options_get_not_found;
		}
		else if(!app->opts_iface->set)
		{
			ret = &ret_options_set_not_found;
		}
		else if(!has_opts_iface)
		{
			ret = &ret_options_interface_not_found;
		}
	}

	return ret;
}

static const ret_t *
_test_options_feature(app_t *app)
{
	static const ret_t ret_options_options_not_found = {
		.lnt = LINT_FAIL,
		.msg = "opts:options not defined",
		.uri = LV2_OPTIONS__options,
		.dsc = "The plugin exposes the options extension, but does not list this "
			"lv2:Feature."
	},
	ret_options_supported_not_found = {
		.lnt = LINT_WARN,
		.msg = "opts:{required,supported} options not defined",
		.uri = LV2_OPTIONS__supportedOption,
		.dsc = "The plugin exposes the options extension, but does not list any "
			"required and/or supported options."
	},
	ret_options_required_found = {
		.lnt = LINT_WARN,
		.msg = "opts:required options defined",
		.uri = LV2_OPTIONS__requiredOption,
		.dsc = "Not all hosts may provide required options, thus make them optional."
	};

	const ret_t *ret = NULL;

	const bool has_opts_options= lilv_plugin_has_feature(app->plugin, NODE(app, OPTIONS__options));
	LilvNodes *required_options = lilv_plugin_get_value(app->plugin, NODE(app, OPTIONS__requiredOption));
	LilvNodes *supported_options = lilv_plugin_get_value(app->plugin, NODE(app, OPTIONS__supportedOption));

	const unsigned required_n = lilv_nodes_size(required_options);
	const unsigned supported_n = lilv_nodes_size(supported_options);
	const unsigned n = required_n + supported_n;

	if(has_opts_options || n)
	{
		if(!has_opts_options)
		{
			ret = &ret_options_options_not_found;
		}
		else if(!n)
		{
			ret = &ret_options_supported_not_found;
		}
	}
	else if(required_n)
	{
		ret = &ret_options_required_found;
	}

	if(required_options)
	{
		lilv_nodes_free(required_options);
	}

	if(supported_options)
	{
		lilv_nodes_free(supported_options);
	}

	return ret;
}

static const ret_t *
_test_uri_map(app_t *app)
{
	static const ret_t ret_uri_map_deprecated = {
		.lnt = LINT_FAIL,
		.msg = "uri-map is deprecated, use urid:map instead",
		.uri = LV2_URI_MAP_URI,
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	if(lilv_plugin_has_feature(app->plugin, NODE(app, URI_MAP)))
	{
		ret = &ret_uri_map_deprecated;
	}

	return ret;
}

static const ret_t *
_test_state(app_t *app)
{
	static const ret_t ret_state_load_default_not_found = {
		.lnt = LINT_FAIL,
		.msg = "state:loadDefaultState not defined",
		.uri = LV2_STATE__loadDefaultState,
		.dsc = "The plugin has a default state, but does not list to load it in its "
			"features."
	},
	ret_state_interface_not_found = {
		.lnt = LINT_FAIL,
		.msg = "state:interface not defined",
		.uri = LV2_STATE__interface,
		.dsc = "The plugin makes use of the state extension, but does not list this "
			"extension data."
	},
	ret_state_state_not_found = {
		.lnt = LINT_WARN,
		.msg = "state:state not defined",
		.uri = LV2_STATE__state,
		.dsc = "The plugin makes use of the state extension, but does not list "
			"a default state."
	},
	ret_state_interface_not_returned = {
		.lnt = LINT_FAIL,
		.msg = "state:interface not returned by 'extension_data'",
		.uri = LV2_STATE__interface,
		.dsc = NULL
	},
	ret_state_save_not_found = {
		.lnt = LINT_FAIL,
		.msg = "state:interface has no 'save' function",
		.uri = LV2_STATE__interface,
		.dsc = NULL
	},
	ret_state_restore_not_found = {
		.lnt = LINT_FAIL,
		.msg = "state:interface has no 'restore' function",
		.uri = LV2_STATE__interface,
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	const bool has_load_default = lilv_plugin_has_feature(app->plugin, NODE(app, STATE__loadDefaultState));
	const bool has_thread_safe_restore = lilv_plugin_has_feature(app->plugin, NODE(app, STATE__threadSafeRestore));
	const bool has_state = lilv_world_ask(app->world,
		lilv_plugin_get_uri(app->plugin), NODE(app, STATE__state), NULL);
	const bool has_iface = lilv_plugin_has_extension_data(app->plugin, NODE(app, STATE__interface));

	if(has_load_default || has_thread_safe_restore || has_state || has_iface || app->state_iface)
	{
		if(!app->state_iface)
		{
			ret = &ret_state_interface_not_returned;
		}
		else if(!app->state_iface->save)
		{
			ret = &ret_state_save_not_found;
		}
		else if(!app->state_iface->restore)
		{
			ret = &ret_state_restore_not_found;
		}
		else if(!has_iface)
		{
			ret = &ret_state_interface_not_found;
		}
		else if(has_load_default || has_state)
		{
			if(!has_load_default)
			{
				ret = &ret_state_load_default_not_found;
			}
			else if(!has_state)
			{
				ret = &ret_state_state_not_found;
			}
		}
	}

	return ret;
}

static const ret_t *
_test_comment(app_t *app)
{
	static const ret_t ret_comment_not_found = {
		.lnt = LINT_NOTE,
		.msg = "rdfs:comment or doap:description not found",
		.uri = LV2_CORE__Plugin,
		.dsc = NULL
	},
	ret_comment_not_a_string = {
		.lnt = LINT_FAIL,
		.msg = "rdfs:comment not a string",
		.uri = LILV_NS_RDFS"comment",
		.dsc = NULL
	},
	ret_description_not_a_string = {
		.lnt = LINT_FAIL,
		.msg = "doap:description not a string",
		.uri = LILV_NS_DOAP"description",
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	LilvNode *comment = lilv_world_get(app->world,
		lilv_plugin_get_uri(app->plugin), NODE(app, RDFS__comment), NULL);
	LilvNode *description= lilv_world_get(app->world,
		lilv_plugin_get_uri(app->plugin), NODE(app, DOAP__description), NULL);

	if(comment)
	{
		if(!lilv_node_is_string(comment))
		{
			ret = &ret_comment_not_a_string;
		}

		lilv_node_free(comment);
		if(description)
			lilv_node_free(description);
	}
	else if(description)
	{
		if(!lilv_node_is_string(description))
		{
			ret = &ret_description_not_a_string;
		}

		lilv_node_free(description);
	}
	else
	{
		ret = &ret_comment_not_found;
	}

	return ret;
}

static const ret_t *
_test_shortdesc(app_t *app)
{
	static const ret_t ret_shortdesc_not_found = {
		.lnt = LINT_NOTE,
		.msg = "doap:shortdesc not found",
		.uri = LILV_NS_DOAP"shortdesc",
		.dsc = NULL
	},
	ret_shortdesc_not_a_string = {
		.lnt = LINT_FAIL,
		.msg = "doap:shortdesc not a string",
		.uri = LILV_NS_DOAP"shortdesc",
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	LilvNode *shortdesc = lilv_world_get(app->world,
		lilv_plugin_get_uri(app->plugin), NODE(app, DOAP__shortdesc), NULL);
	if(shortdesc)
	{
		if(!lilv_node_is_string(shortdesc))
		{
			ret = &ret_shortdesc_not_a_string;
		}

		lilv_node_free(shortdesc);
	}
	else
	{
		ret = &ret_shortdesc_not_found;
	}

	return ret;
}

static const ret_t *
_test_idisp(app_t *app)
{
	static const ret_t ret_idisp_queue_draw_not_found = {
		.lnt = LINT_FAIL,
		.msg = "idisp:queue_draw not defined",
		.uri = LV2_INLINEDISPLAY__queue_draw,
		.dsc = "The plugin makes use of the inline display extension, but does not "
			"list this feature."
	},
	ret_idisp_interface_not_found = {
		.lnt = LINT_FAIL,
		.msg = "idisp:interface not defined",
		.uri = LV2_INLINEDISPLAY__interface,
		.dsc = "The plugin makes use of the inline display extension, but does not "
			"list this extension data."
	},
	ret_idisp_interface_not_returned = {
		.lnt = LINT_FAIL,
		.msg = "idisp:interface not returned by 'extention_data'",
		.uri = LV2_INLINEDISPLAY__interface,
		.dsc = NULL
	},
	ret_idisp_render_not_found = {
		.lnt = LINT_FAIL,
		.msg ="idisp:interface has no 'render' function",
		.uri = LV2_INLINEDISPLAY__interface,
		.dsc = NULL
	};

	const ret_t *ret = NULL;

	const bool has_idisp_queue_draw = lilv_plugin_has_feature(app->plugin, NODE(app, INLINEDISPLAY__queue_draw));
	const bool has_idisp_iface = lilv_plugin_has_extension_data(app->plugin, NODE(app, INLINEDISPLAY__interface));

	if(has_idisp_queue_draw || has_idisp_iface || app->idisp_iface)
	{
		if(!app->idisp_iface)
		{
			ret = &ret_idisp_interface_not_returned;
		}
		else if(!app->idisp_iface->render)
		{
			ret = &ret_idisp_render_not_found;
		}
		else if(!has_idisp_queue_draw)
		{
			ret = &ret_idisp_queue_draw_not_found;
		}
		else if(!has_idisp_iface)
		{
			ret = &ret_idisp_interface_not_found;
		}
	}

	return ret;
}

static const ret_t *
_test_hard_rt_capable(app_t *app)
{
	static const ret_t ret_hard_rt_capable_not_found = {
		.lnt = LINT_WARN,
		.msg = "not advertized as real-time safe",
		.uri = LV2_CORE__hardRTCapable,
		.dsc = "If this plugin is meant to be used in a real-time context, you "
			"should list this feature."
	};

	const ret_t *ret = NULL;

	const bool is_hard_rt_capable = lilv_plugin_has_feature(app->plugin, NODE(app, CORE__hardRTCapable));

	if(!is_hard_rt_capable)
	{
		ret = &ret_hard_rt_capable_not_found;
	}

	return ret;
}

static const ret_t *
_test_in_place_broken(app_t *app)
{
	static const ret_t ret_in_place_broken_found = {
		.lnt = LINT_WARN,
		.msg = "cannot process audio/CV in-place",
		.uri = LV2_CORE__inPlaceBroken,
		.dsc = "Some hosts only support plugins that are capable of in-place "
			"processing."
	};

	const ret_t *ret = NULL;

	const bool is_in_place_broken = lilv_plugin_has_feature(app->plugin, NODE(app, CORE__inPlaceBroken));

	if(is_in_place_broken)
	{
		ret = &ret_in_place_broken_found;
	}

	return ret;
}

static const ret_t *
_test_is_live(app_t *app)
{
	static const ret_t ret_is_live_not_found = {
		.lnt = LINT_NOTE,
		.msg = "not meant for live usage",
		.uri = LV2_CORE__isLive,
		.dsc = "If this plugin is meant to be used in a live context, you "
			"should list this feature."
	};

	const ret_t *ret = NULL;

	const bool is_live = lilv_plugin_has_feature(app->plugin, NODE(app, CORE__isLive));

	if(!is_live)
	{
		ret = &ret_is_live_not_found;
	}

	return ret;
}

static const ret_t *
_test_fixed_block_length(app_t *app)
{
	static const ret_t ret_fixed_block_length_found = {
		.lnt = LINT_WARN,
		.msg = "requiring a fixed block length is highly discouraged",
		.uri = LV2_BUF_SIZE__fixedBlockLength,
		.dsc = "Some hosts do not support fixed block lengths, try to avoid this."
	};

	const ret_t *ret = NULL;

	const bool wants_fixed_block_length =
		lilv_plugin_has_feature(app->plugin, NODE(app, BUF_SIZE__fixedBlockLength));

	if(wants_fixed_block_length)
	{
		ret = &ret_fixed_block_length_found;
	}

	return ret;
}

static const ret_t *
_test_power_of_2_block_length(app_t *app)
{
	static const ret_t ret_power_of_2_block_length_found = {
		.lnt = LINT_WARN,
		.msg = "requiring a power of 2 block length is highly discouraged",
		.uri = LV2_BUF_SIZE__powerOf2BlockLength,
		.dsc = "Some hosts do not support power of 2 block lengths, tr to avoid this."
	};

	const ret_t *ret = NULL;

	const bool wants_power_of_2_block_length =
		lilv_plugin_has_feature(app->plugin, NODE(app, BUF_SIZE__powerOf2BlockLength));

	if(wants_power_of_2_block_length)
	{
		ret = &ret_power_of_2_block_length_found;
	}

	return ret;
}

#ifdef ENABLE_ONLINE_TESTS
static const ret_t *
_test_plugin_url(app_t *app)
{
	static const ret_t ret_plugin_url_not_existing = {
		.lnt = LINT_WARN,
		.msg = "Plugin Web URL does not exist",
		.uri = LV2_CORE__Plugin,
		.dsc = "A plugin URI ideally links to an existing Web page with further "
			"documentation."
	};

	const ret_t *ret = NULL;

	const char *uri = lilv_node_as_uri(lilv_plugin_get_uri(app->plugin));

	if(is_url(uri))
	{
		const bool url_exists = !app->online || test_url(app, uri);

		if(!url_exists)
		{
			ret = &ret_plugin_url_not_existing;
		}
	}

	return ret;
}
#endif

static const ret_t *
_test_patch(app_t *app)
{
	static const ret_t ret_patch_no_patch_message_support_on_output = {
		.lnt = LINT_FAIL,
		.msg = "no patch:Message support on any output",
		.uri = LV2_PATCH__Message,
		.dsc = "The plugin lists parameters, but has no output port assigned to "
			"patch messages."
	},
	ret_patch_no_patch_message_support_on_input = {
		.lnt = LINT_FAIL,
		.msg = "no patch:Message support on any input",
		.uri = LV2_PATCH__Message,
		.dsc = "The plugin lists parameters, but has no input port assigned to "
			"patch messages."
	},
	ret_patch_no_parameters_found = {
		.lnt = LINT_NOTE,
		.msg = "no patch:writable/readable parameters found",
		.uri = LV2_PATCH__writable,
		.dsc = "The plugin lists ports assigned to route patch messages, but has "
			"no writable or readlable parameters listed."
	};

	const ret_t *ret = NULL;

	const unsigned n_writables = app->writables
		? lilv_nodes_size(app->writables) : 0;
	const unsigned n_readables = app->readables
		? lilv_nodes_size(app->readables) : 0;
	const unsigned n_parameters = n_writables + n_readables;

	unsigned n_patch_message_input = 0;
	unsigned n_patch_message_output = 0;

	const uint32_t num_ports = lilv_plugin_get_num_ports(app->plugin);
	for(unsigned i=0; i<num_ports; i++)
	{
		const LilvPort *port = lilv_plugin_get_port_by_index(app->plugin, i);

		if(  lilv_port_is_a(app->plugin, port, NODE(app, ATOM__AtomPort))
			&& lilv_port_supports_event(app->plugin, port, NODE(app, PATCH__Message)) )
		{
			if(lilv_port_is_a(app->plugin, port, NODE(app, CORE__InputPort)))
			{
				n_patch_message_input += 1;
			}
			else if(lilv_port_is_a(app->plugin, port, NODE(app, CORE__OutputPort)))
			{
				n_patch_message_output += 1;
			}
		}
	}

	if(n_parameters + n_patch_message_input + n_patch_message_output)
	{
		if(n_parameters == 0)
		{
			ret = &ret_patch_no_parameters_found;
		}
		else if(n_patch_message_input == 0)
		{
			ret = &ret_patch_no_patch_message_support_on_input;
		}
		else if(n_patch_message_output == 0)
		{
			ret = &ret_patch_no_patch_message_support_on_output;
		}
	}

	return ret;
}

static const test_t tests [] = {
	{"Plugin LV2_PATH",        _test_lv2_path},
	{"Plugin Instantiation",   _test_instantiation},
#ifdef ENABLE_ELF_TESTS
	{"Plugin Symbols",         _test_symbols},
	{"Plugin Fork",            _test_fork},
	{"Plugin Linking",         _test_linking},
#endif
	{"Plugin Verification",    _test_verification},
	{"Plugin Name",            _test_name},
	{"Plugin License",         _test_license},
	{"Plugin Author Name",     _test_author_name},
	{"Plugin Author Email",    _test_author_email},
	{"Plugin Author Homepage", _test_author_homepage},
	{"Plugin Version Minor",   _test_version_minor},
	{"Plugin Version Micro",   _test_version_micro},
	{"Plugin Project",         _test_project},
	{"Plugin Class",           _test_class},
	{"Plugin Features",        _test_features},
	{"Plugin Extension Data",  _test_extensions},
	{"Plugin Worker",          _test_worker},
	{"Plugin Options Iface",   _test_options_iface},
	{"Plugin Options Feature", _test_options_feature},
	{"Plugin URI-Map",         _test_uri_map},
	{"Plugin State",           _test_state},
	{"Plugin Comment",         _test_comment},
	{"Plugin Shortdesc",       _test_shortdesc},
	{"Plugin Inline Display",  _test_idisp},
	{"Plugin Hard RT Capable", _test_hard_rt_capable},
	{"Plugin In Place Broken", _test_in_place_broken},
	{"Plugin Is Live",         _test_is_live},
	//{"Plugin Bounded Block",   _test_bounded_block_length}, //TODO check for opts:opt
	{"Plugin Fixed Block",     _test_fixed_block_length},
	{"Plugin PowerOf2 Block",  _test_power_of_2_block_length},
#ifdef ENABLE_ONLINE_TESTS
	{"Plugin URL",             _test_plugin_url},
#endif
	{"Plugin Patch",           _test_patch},
};

static const unsigned tests_n = sizeof(tests) / sizeof(test_t);

bool
test_plugin(app_t *app)
{
	bool flag = true;
	bool msg = false;
	res_t *rets = alloca(tests_n * sizeof(res_t));
	if(!rets)
		return flag;

	app->writables = lilv_plugin_get_value(app->plugin, NODE(app, PATCH__writable));
	app->readables = lilv_plugin_get_value(app->plugin, NODE(app, PATCH__readable));

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

	const uint32_t num_ports = lilv_plugin_get_num_ports(app->plugin);
	for(unsigned i=0; i<num_ports; i++)
	{
		bool port_flag = true;

		app->port = lilv_plugin_get_port_by_index(app->plugin, i);
		if(app->port)
		{
			if(!test_port(app))
				port_flag = false;
			app->port = NULL;
		}
		else
			port_flag = false;

		if(flag && !port_flag)
			flag = port_flag;
	}

	if(app->writables)
	{
		LILV_FOREACH(nodes, itr, app->writables)
		{
			bool param_flag = true;

			app->parameter = lilv_nodes_get(app->writables, itr);
			if(app->parameter)
			{
				if(!test_parameter(app))
					param_flag = false;
				app->parameter = NULL;
			}
			else
				param_flag = false;

			if(flag && !param_flag)
				flag = param_flag;
		}

		lilv_nodes_free(app->writables);
		app->writables = NULL;
	}

	if(app->readables)
	{
		LILV_FOREACH(nodes, itr, app->readables)
		{
			bool param_flag = true;

			app->parameter = lilv_nodes_get(app->readables, itr);
			if(app->parameter)
			{
				if(!test_parameter(app))
					param_flag = false;
				app->parameter = NULL;
			}
			else
				param_flag = false;

			if(flag && !param_flag)
				flag = param_flag;
		}

		lilv_nodes_free(app->readables);
		app->readables = NULL;
	}

	LilvUIs *uis = lilv_plugin_get_uis(app->plugin);
	if(uis)
	{
		LILV_FOREACH(uis, itr, uis)
		{
			bool ui_flag = true;

			app->ui = lilv_uis_get(uis, itr);
			if(app->ui)
			{
				const LilvNode *ui_uri = lilv_ui_get_uri(app->ui);
				lilv_world_load_resource(app->world, ui_uri);

				if(!test_ui(app))
					ui_flag = false;
				app->ui = NULL;

				lilv_world_unload_resource(app->world, ui_uri);
			}
			else
				ui_flag = false;

			if(flag && !ui_flag)
				flag = ui_flag;
		}

		lilv_uis_free(uis);
	}

	lv2lint_printf(app, "\n");

	return flag;
}
