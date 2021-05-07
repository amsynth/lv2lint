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

#include <lv2/patch/patch.h>
#include <lv2/atom/atom.h>
#include <lv2/worker/worker.h>
#include <lv2/log/log.h>
#include <lv2/port-groups/port-groups.h>
#include <lv2/event/event.h>
#include <lv2/morph/morph.h>
#include <lv2/uri-map/uri-map.h>
#include <lv2/instance-access/instance-access.h>
#include <lv2/parameters/parameters.h>
#include <lv2/port-props/port-props.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/resize-port/resize-port.h>
#include <lv2/options/options.h>
#include <lv2/data-access/data-access.h>
#include <lv2/dynmanifest/dynmanifest.h>
#include <lv2/state/state.h>
#include <lv2/ui/ui.h>
#include <lv2/units/units.h>

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

#define NS_ITM(EXT, ID) [EXT ## __ ## ID] = LILV_NS_ ## EXT # ID
#define ITM(ID) [ID] = LV2_ ## ID

static const char *stat_uris [STAT_URID_MAX] = {
	[STAT_URID_INVALID] = NULL,

	NS_ITM(RDFS, label),
	NS_ITM(RDFS, comment),
	NS_ITM(RDFS, range),
	NS_ITM(RDFS, subClassOf),

	NS_ITM(RDF, type),
	NS_ITM(RDF, value),

	NS_ITM(DOAP, description),
	NS_ITM(DOAP, license),
	NS_ITM(DOAP, name),
	NS_ITM(DOAP, shortdesc),

	NS_ITM(XSD, int),
	NS_ITM(XSD, nonNegativeInteger),
	NS_ITM(XSD, long),
	NS_ITM(XSD, float),
	NS_ITM(XSD, double),

	ITM(ATOM__Atom),
	ITM(ATOM__AtomPort),
	ITM(ATOM__Blank),
	ITM(ATOM__Bool),
	ITM(ATOM__Chunk),
	ITM(ATOM__Double),
	ITM(ATOM__Event),
	ITM(ATOM__Float),
	ITM(ATOM__Int),
	ITM(ATOM__Literal),
	ITM(ATOM__Long),
	ITM(ATOM__Number),
	ITM(ATOM__Object),
	ITM(ATOM__Path),
	ITM(ATOM__Property),
	ITM(ATOM__Resource),
	ITM(ATOM__Sequence),
	ITM(ATOM__Sound),
	ITM(ATOM__String),
	ITM(ATOM__Tuple),
	ITM(ATOM__URI),
	ITM(ATOM__URID),
	ITM(ATOM__Vector),
	ITM(ATOM__atomTransfer),
	ITM(ATOM__beatTime),
	ITM(ATOM__bufferType),
	ITM(ATOM__childType),
	ITM(ATOM__eventTransfer),
	ITM(ATOM__frameTime),
	ITM(ATOM__supports),
	ITM(ATOM__timeUnit),

	ITM(BUF_SIZE__boundedBlockLength),
	ITM(BUF_SIZE__coarseBlockLength),
	ITM(BUF_SIZE__fixedBlockLength),
	ITM(BUF_SIZE__maxBlockLength),
	ITM(BUF_SIZE__minBlockLength),
	ITM(BUF_SIZE__nominalBlockLength),
	ITM(BUF_SIZE__powerOf2BlockLength),
	ITM(BUF_SIZE__sequenceSize),

	ITM(CORE__AllpassPlugin),
	ITM(CORE__AmplifierPlugin),
	ITM(CORE__AnalyserPlugin),
	ITM(CORE__AudioPort),
	ITM(CORE__BandpassPlugin),
	ITM(CORE__CVPort),
	ITM(CORE__ChorusPlugin),
	ITM(CORE__CombPlugin),
	ITM(CORE__CompressorPlugin),
	ITM(CORE__ConstantPlugin),
	ITM(CORE__ControlPort),
	ITM(CORE__ConverterPlugin),
	ITM(CORE__DelayPlugin),
	ITM(CORE__DistortionPlugin),
	ITM(CORE__DynamicsPlugin),
	ITM(CORE__EQPlugin),
	ITM(CORE__EnvelopePlugin),
	ITM(CORE__ExpanderPlugin),
	ITM(CORE__ExtensionData),
	ITM(CORE__Feature),
	ITM(CORE__FilterPlugin),
	ITM(CORE__FlangerPlugin),
	ITM(CORE__FunctionPlugin),
	ITM(CORE__GatePlugin),
	ITM(CORE__GeneratorPlugin),
	ITM(CORE__HighpassPlugin),
	ITM(CORE__InputPort),
	ITM(CORE__InstrumentPlugin),
	ITM(CORE__LimiterPlugin),
	ITM(CORE__LowpassPlugin),
	ITM(CORE__MixerPlugin),
	ITM(CORE__ModulatorPlugin),
	ITM(CORE__MultiEQPlugin),
	ITM(CORE__OscillatorPlugin),
	ITM(CORE__OutputPort),
	ITM(CORE__ParaEQPlugin),
	ITM(CORE__PhaserPlugin),
	ITM(CORE__PitchPlugin),
	ITM(CORE__Plugin),
	ITM(CORE__PluginBase),
	ITM(CORE__Point),
	ITM(CORE__Port),
	ITM(CORE__PortProperty),
	ITM(CORE__Resource),
	ITM(CORE__ReverbPlugin),
	ITM(CORE__ScalePoint),
	ITM(CORE__SimulatorPlugin),
	ITM(CORE__SpatialPlugin),
	ITM(CORE__Specification),
	ITM(CORE__SpectralPlugin),
	ITM(CORE__UtilityPlugin),
	ITM(CORE__WaveshaperPlugin),
	ITM(CORE__appliesTo),
	ITM(CORE__binary),
	ITM(CORE__connectionOptional),
	ITM(CORE__control),
	ITM(CORE__default),
	ITM(CORE__designation),
	ITM(CORE__documentation),
	ITM(CORE__enumeration),
	ITM(CORE__extensionData),
	ITM(CORE__freeWheeling),
	ITM(CORE__hardRTCapable),
	ITM(CORE__inPlaceBroken),
	ITM(CORE__index),
	ITM(CORE__integer),
	ITM(CORE__isLive),
	ITM(CORE__latency),
	ITM(CORE__maximum),
	ITM(CORE__microVersion),
	ITM(CORE__minimum),
	ITM(CORE__minorVersion),
	ITM(CORE__name),
	ITM(CORE__optionalFeature),
	ITM(CORE__port),
	ITM(CORE__portProperty),
	ITM(CORE__project),
	ITM(CORE__prototype),
	ITM(CORE__reportsLatency),
	ITM(CORE__requiredFeature),
	ITM(CORE__sampleRate),
	ITM(CORE__scalePoint),
	ITM(CORE__symbol),
	ITM(CORE__toggled),

	[DATA_ACCESS] = LV2_DATA_ACCESS_URI,

	[DYN_MANIFEST] = LV2_DYN_MANIFEST_URI,

	ITM(EVENT__Event),
	ITM(EVENT__EventPort),
	ITM(EVENT__FrameStamp),
	ITM(EVENT__TimeStamp),
	ITM(EVENT__generatesTimeStamp),
	ITM(EVENT__generic),
	ITM(EVENT__inheritsEvent),
	ITM(EVENT__inheritsTimeStamp),
	ITM(EVENT__supportsEvent),
	ITM(EVENT__supportsTimeStamp),

	[INSTANCE_ACCESS] = LV2_INSTANCE_ACCESS_URI,

	ITM(LOG__Entry),
	ITM(LOG__Error),
	ITM(LOG__Note),
	ITM(LOG__Trace),
	ITM(LOG__Warning),
	ITM(LOG__log),

	ITM(MIDI__ActiveSense),
	ITM(MIDI__Aftertouch),
	ITM(MIDI__Bender),
	ITM(MIDI__ChannelPressure),
	ITM(MIDI__Chunk),
	ITM(MIDI__Clock),
	ITM(MIDI__Continue),
	ITM(MIDI__Controller),
	ITM(MIDI__MidiEvent),
	ITM(MIDI__NoteOff),
	ITM(MIDI__NoteOn),
	ITM(MIDI__ProgramChange),
	ITM(MIDI__QuarterFrame),
	ITM(MIDI__Reset),
	ITM(MIDI__SongPosition),
	ITM(MIDI__SongSelect),
	ITM(MIDI__Start),
	ITM(MIDI__Stop),
	ITM(MIDI__SystemCommon),
	ITM(MIDI__SystemExclusive),
	ITM(MIDI__SystemMessage),
	ITM(MIDI__SystemRealtime),
	ITM(MIDI__Tick),
	ITM(MIDI__TuneRequest),
	ITM(MIDI__VoiceMessage),
	ITM(MIDI__benderValue),
	ITM(MIDI__binding),
	ITM(MIDI__byteNumber),
	ITM(MIDI__channel),
	ITM(MIDI__chunk),
	ITM(MIDI__controllerNumber),
	ITM(MIDI__controllerValue),
	ITM(MIDI__noteNumber),
	ITM(MIDI__pressure),
	ITM(MIDI__programNumber),
	ITM(MIDI__property),
	ITM(MIDI__songNumber),
	ITM(MIDI__songPosition),
	ITM(MIDI__status),
	ITM(MIDI__statusMask),
	ITM(MIDI__velocity),

	ITM(MORPH__AutoMorphPort),
	ITM(MORPH__MorphPort),
	ITM(MORPH__interface),
	ITM(MORPH__supportsType),
	ITM(MORPH__currentType),

	ITM(OPTIONS__Option),
	ITM(OPTIONS__interface),
	ITM(OPTIONS__options),
	ITM(OPTIONS__requiredOption),
	ITM(OPTIONS__supportedOption),

	ITM(PARAMETERS__CompressorControls),
	ITM(PARAMETERS__ControlGroup),
	ITM(PARAMETERS__EnvelopeControls),
	ITM(PARAMETERS__FilterControls),
	ITM(PARAMETERS__OscillatorControls),
	ITM(PARAMETERS__amplitude),
	ITM(PARAMETERS__attack),
	ITM(PARAMETERS__bypass),
	ITM(PARAMETERS__cutoffFrequency),
	ITM(PARAMETERS__decay),
	ITM(PARAMETERS__delay),
	ITM(PARAMETERS__dryLevel),
	ITM(PARAMETERS__frequency),
	ITM(PARAMETERS__gain),
	ITM(PARAMETERS__hold),
	ITM(PARAMETERS__pulseWidth),
	ITM(PARAMETERS__ratio),
	ITM(PARAMETERS__release),
	ITM(PARAMETERS__resonance),
	ITM(PARAMETERS__sampleRate),
	ITM(PARAMETERS__sustain),
	ITM(PARAMETERS__threshold),
	ITM(PARAMETERS__waveform),
	ITM(PARAMETERS__wetDryRatio),
	ITM(PARAMETERS__wetLevel),

	ITM(PATCH__Ack),
	ITM(PATCH__Delete),
	ITM(PATCH__Copy),
	ITM(PATCH__Error),
	ITM(PATCH__Get),
	ITM(PATCH__Message),
	ITM(PATCH__Move),
	ITM(PATCH__Patch),
	ITM(PATCH__Post),
	ITM(PATCH__Put),
	ITM(PATCH__Request),
	ITM(PATCH__Response),
	ITM(PATCH__Set),
	ITM(PATCH__accept),
	ITM(PATCH__add),
	ITM(PATCH__body),
	ITM(PATCH__context),
	ITM(PATCH__destination),
	ITM(PATCH__property),
	ITM(PATCH__readable),
	ITM(PATCH__remove),
	ITM(PATCH__request),
	ITM(PATCH__subject),
	ITM(PATCH__sequenceNumber),
	ITM(PATCH__value),
	ITM(PATCH__wildcard),
	ITM(PATCH__writable),

	ITM(PORT_GROUPS__DiscreteGroup),
	ITM(PORT_GROUPS__Element),
	ITM(PORT_GROUPS__FivePointOneGroup),
	ITM(PORT_GROUPS__FivePointZeroGroup),
	ITM(PORT_GROUPS__FourPointZeroGroup),
	ITM(PORT_GROUPS__Group),
	ITM(PORT_GROUPS__InputGroup),
	ITM(PORT_GROUPS__MidSideGroup),
	ITM(PORT_GROUPS__MonoGroup),
	ITM(PORT_GROUPS__OutputGroup),
	ITM(PORT_GROUPS__SevenPointOneGroup),
	ITM(PORT_GROUPS__SevenPointOneWideGroup),
	ITM(PORT_GROUPS__SixPointOneGroup),
	ITM(PORT_GROUPS__StereoGroup),
	ITM(PORT_GROUPS__ThreePointZeroGroup),
	ITM(PORT_GROUPS__center),
	ITM(PORT_GROUPS__centerLeft),
	ITM(PORT_GROUPS__centerRight),
	ITM(PORT_GROUPS__element),
	ITM(PORT_GROUPS__group),
	ITM(PORT_GROUPS__left),
	ITM(PORT_GROUPS__lowFrequencyEffects),
	ITM(PORT_GROUPS__mainInput),
	ITM(PORT_GROUPS__mainOutput),
	ITM(PORT_GROUPS__rearCenter),
	ITM(PORT_GROUPS__rearLeft),
	ITM(PORT_GROUPS__rearRight),
	ITM(PORT_GROUPS__right),
	ITM(PORT_GROUPS__side),
	ITM(PORT_GROUPS__sideChainOf),
	ITM(PORT_GROUPS__sideLeft),
	ITM(PORT_GROUPS__sideRight),
	ITM(PORT_GROUPS__source),
	ITM(PORT_GROUPS__subGroupOf),

	ITM(PORT_PROPS__causesArtifacts),
	ITM(PORT_PROPS__continuousCV),
	ITM(PORT_PROPS__discreteCV),
	ITM(PORT_PROPS__displayPriority),
	ITM(PORT_PROPS__expensive),
	ITM(PORT_PROPS__hasStrictBounds),
	ITM(PORT_PROPS__logarithmic),
	ITM(PORT_PROPS__notAutomatic),
	ITM(PORT_PROPS__notOnGUI),
	ITM(PORT_PROPS__rangeSteps),
	ITM(PORT_PROPS__supportsStrictBounds),
	ITM(PORT_PROPS__trigger),

	ITM(PRESETS__Bank),
	ITM(PRESETS__Preset),
	ITM(PRESETS__bank),
	ITM(PRESETS__preset),
	ITM(PRESETS__value),

	ITM(RESIZE_PORT__asLargeAs),
	ITM(RESIZE_PORT__minimumSize),
	ITM(RESIZE_PORT__resize),

	ITM(STATE__State),
	ITM(STATE__interface),
	ITM(STATE__loadDefaultState),
	ITM(STATE__freePath),
	ITM(STATE__makePath),
	ITM(STATE__mapPath),
	ITM(STATE__state),
	ITM(STATE__threadSafeRestore),
	ITM(STATE__StateChanged),

	ITM(TIME__Time),
	ITM(TIME__Position),
	ITM(TIME__Rate),
	ITM(TIME__position),
	ITM(TIME__barBeat),
	ITM(TIME__bar),
	ITM(TIME__beat),
	ITM(TIME__beatUnit),
	ITM(TIME__beatsPerBar),
	ITM(TIME__beatsPerMinute),
	ITM(TIME__frame),
	ITM(TIME__framesPerSecond),
	ITM(TIME__speed),

	ITM(UI__CocoaUI),
	ITM(UI__Gtk3UI),
	ITM(UI__GtkUI),
	ITM(UI__PortNotification),
	ITM(UI__PortProtocol),
	ITM(UI__Qt4UI),
	ITM(UI__Qt5UI),
	ITM(UI__UI),
	ITM(UI__WindowsUI),
	ITM(UI__X11UI),
	ITM(UI__binary),
	ITM(UI__fixedSize),
	ITM(UI__idleInterface),
	ITM(UI__noUserResize),
	ITM(UI__notifyType),
	ITM(UI__parent),
	ITM(UI__plugin),
	ITM(UI__portIndex),
	ITM(UI__portMap),
	ITM(UI__portNotification),
	ITM(UI__portSubscribe),
	ITM(UI__protocol),
	ITM(UI__requestValue),
	ITM(UI__floatProtocol),
	ITM(UI__peakProtocol),
	ITM(UI__resize),
	ITM(UI__showInterface),
	ITM(UI__touch),
	ITM(UI__ui),
	ITM(UI__updateRate),
	ITM(UI__windowTitle),
	ITM(UI__scaleFactor),
	ITM(UI__foregroundColor),
	ITM(UI__backgroundColor),
	ITM(UI__makeSONameResident),

	ITM(UNITS__Conversion),
	ITM(UNITS__Unit),
	ITM(UNITS__bar),
	ITM(UNITS__beat),
	ITM(UNITS__bpm),
	ITM(UNITS__cent),
	ITM(UNITS__cm),
	ITM(UNITS__coef),
	ITM(UNITS__conversion),
	ITM(UNITS__db),
	ITM(UNITS__degree),
	ITM(UNITS__frame),
	ITM(UNITS__hz),
	ITM(UNITS__inch),
	ITM(UNITS__khz),
	ITM(UNITS__km),
	ITM(UNITS__m),
	ITM(UNITS__mhz),
	ITM(UNITS__midiNote),
	ITM(UNITS__mile),
	ITM(UNITS__min),
	ITM(UNITS__mm),
	ITM(UNITS__ms),
	ITM(UNITS__name),
	ITM(UNITS__oct),
	ITM(UNITS__pc),
	ITM(UNITS__prefixConversion),
	ITM(UNITS__render),
	ITM(UNITS__s),
	ITM(UNITS__semitone12TET),
	ITM(UNITS__symbol),
	ITM(UNITS__unit),

	ITM(URID__map),
	ITM(URID__unmap),

	[URI_MAP] = LV2_URI_MAP_URI,

	ITM(WORKER__interface),
	ITM(WORKER__schedule),

	ITM(EXTERNAL_UI__Widget),

	ITM(INLINEDISPLAY__interface),
	ITM(INLINEDISPLAY__queue_draw)
};

#undef ITM

static void
_map_uris(app_t *app)
{
	for(unsigned i = 1; i < STAT_URID_MAX; i++)
	{
		app->nodes[i] = lilv_new_uri(app->world, stat_uris[i]);
	}
}

static void
_unmap_uris(app_t *app)
{
	for(unsigned i = 1; i < STAT_URID_MAX; i++)
	{
		lilv_node_free(app->nodes[i]);
	}
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

int
log_vprintf(void *data __unused, LV2_URID type __unused, const char *fmt,
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

int
log_printf(void *data, LV2_URID type, const char *fmt, ...)
{
  va_list args;
	int ret;

  va_start (args, fmt);
	ret = log_vprintf(data, type, fmt, args);
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

static void
_freepath(LV2_State_Free_Path_Handle instance __unused, char *absolute_path)
{
	if(absolute_path)
	{
		free(absolute_path);
	}
}

static LV2_Resize_Port_Status
_resize(LV2_Resize_Port_Feature_Data instance __unused, uint32_t index __unused,
	size_t size __unused)
{
	return LV2_RESIZE_PORT_SUCCESS;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
uint32_t
uri_to_id(LV2_URI_Map_Callback_Data instance,
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
		"Copyright (c) 2016-2021 Hanspeter Portner (dev@open-music-kontrollers.ch)\n"
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
		"   [-u] URI_PATTERN             URI pattern (shell wildcards) to prefix other whitelist patterns "
		                                 " (can be used multiple times)\n"
		"   [-t] TEST_PATTERN            test name pattern (shell wildcards) to whitelist"
		                                 " (can be used multiple times)\n"
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
	if(fnmatch(pattern, str, FNM_CASEFOLD) == 0)
#else
	if(strcasecmp(pattern, str) == 0)
#endif
	{
		return true;
	}

	return false;
}

static bool
_white_match(const white_t *white, const char *uri, const char *str)
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
test_visibility(app_t *app, const char *path, const char *uri,
	const char *description, char **symbols)
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
		"__rt_data__start",
		"__rt_data__end",
		"__rt_text__start",
		"__rt_text__end",
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
									if(_white_match(app->whitelist_symbols, uri, name))
									{
										whitelist_match = true;
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
check_for_symbol(app_t *app __attribute__((unused)), const char *path,
	const char *description)
{
	bool desc = false;

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

						const char *name = elf_strptr(elf, shdr.sh_link, sym.st_name);
						if(!strcmp(name, description))
						{
							desc = true;
							break;
						}
					}
				}
			}
			elf_end(elf);
		}
		close(fd);
	}

	return desc;
}

bool
test_shared_libraries(app_t *app, const char *path, const char *uri,
	const char *const *whitelist, unsigned n_whitelist,
	const char *const *blacklist, unsigned n_blacklist,
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

							if(_white_match(app->whitelist_libs, uri, name))
							{
								whitelist_match = true;
								break;
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
_append_whitelist_symbol(app_t *app, const char *uri, char *pattern)
{
	app->whitelist_symbols = _white_append(app->whitelist_symbols, uri, pattern);
}

static void
_free_whitelist_symbols(app_t *app)
{
	app->whitelist_symbols = _white_free(app->whitelist_symbols);
}

static void
_append_whitelist_lib(app_t *app, const char *uri, char *pattern)
{
	app->whitelist_libs = _white_append(app->whitelist_libs, uri, pattern);
}

static void
_free_whitelist_libs(app_t *app)
{
	app->whitelist_libs = _white_free(app->whitelist_libs);
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
				_append_whitelist_symbol(&app, uri, optarg);
				break;
			case 'l':
				_append_whitelist_lib(&app, uri, optarg);
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

	mapper_t *mapper = mapper_new(8192, STAT_URID_MAX, stat_uris, NULL, NULL, NULL);
	if(!mapper)
		return -1;

	_map_uris(&app);
	lilv_world_load_all(app.world);
	_load_include_dirs(&app);

	app.map = mapper_get_map(mapper);
	app.unmap = mapper_get_unmap(mapper);
	LV2_Worker_Schedule sched = {
		.handle = &app,
		.schedule_work = _sched
	};
	LV2_Log_Log log = {
		.handle = &app,
		.printf = log_printf,
		.vprintf = log_vprintf
	};
	LV2_State_Make_Path mkpath = {
		.handle = &app,
		.path = _mkpath
	};
	LV2_State_Free_Path freepath = {
		.handle = &app,
		.free_path = _freepath
	};
	LV2_Resize_Port_Resize rsz = {
		.data = &app,
		.resize = _resize
	};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	LV2_URI_Map_Feature urimap = {
		.callback_data = app.map,
		.uri_to_id = uri_to_id
	};
#pragma GCC diagnostic pop
	LV2_Inline_Display queue_draw = {
		.handle = &app,
		.queue_draw = _queue_draw
	};

	const float param_sample_rate = 48000.f;
	const float ui_update_rate = 25.f;
	const int32_t bufsz_min_block_length = 256;
	const int32_t bufsz_max_block_length = 256;
	const int32_t bufsz_nominal_block_length = 256;
	const int32_t bufsz_sequence_size = 2048;

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

	const LV2_Options_Option opts_minBlockLength = {
		.key = BUF_SIZE__minBlockLength,
		.size = sizeof(int32_t),
		.type = ATOM__Int,
		.value = &bufsz_min_block_length
	};

	const LV2_Options_Option opts_maxBlockLength = {
		.key = BUF_SIZE__maxBlockLength,
		.size = sizeof(int32_t),
		.type = ATOM__Int,
		.value = &bufsz_max_block_length
	};

	const LV2_Options_Option opts_nominalBlockLength = {
		.key = BUF_SIZE__nominalBlockLength,
		.size = sizeof(int32_t),
		.type = ATOM__Int,
		.value = &bufsz_nominal_block_length
	};

	const LV2_Options_Option opts_sequenceSize = {
		.key = BUF_SIZE__sequenceSize,
		.size = sizeof(int32_t),
		.type = ATOM__Int,
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
		.data = app.map
	};
	const LV2_Feature feat_unmap = {
		.URI = LV2_URID__unmap,
		.data = app.unmap
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
	const LV2_Feature feat_freepath = {
		.URI = LV2_STATE__freePath,
		.data = &freepath
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
#define MAX_FEATURES 21
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
								const LV2_URID feat = app.map->map(app.map->handle, lilv_node_as_uri(feature));

								switch(feat)
								{
									case URID__map:
									{
										features[f++] = &feat_map;
									}	break;
									case URID__unmap:
									{
										features[f++] = &feat_unmap;
									}	break;
									case WORKER__schedule:
									{
										features[f++] = &feat_sched;
									}	break;
									case LOG__log:
									{
										features[f++] = &feat_log;
									}	break;
									case STATE__makePath:
									{
										features[f++] = &feat_mkpath;
									}	break;
									case STATE__freePath:
									{
										features[f++] = &feat_freepath;
									}	break;
									case UI__resize:
									{
										features[f++] = &feat_rsz;
									}	break;
									case OPTIONS__options:
									{
										features[f++] = &feat_opts;
									}	break;
									case URI_MAP:
									{
										features[f++] = &feat_urimap;
									}	break;
									case CORE__isLive:
									{
										features[f++] = &feat_islive;
									}	break;
									case CORE__inPlaceBroken:
									{
										features[f++] = &feat_inplacebroken;
									}	break;
									case CORE__hardRTCapable:
									{
										features[f++] = &feat_hardrtcapable;
									}	break;
									case PORT_PROPS__supportsStrictBounds:
									{
										features[f++] = &feat_supportsstrictbounds;
									}	break;
									case BUF_SIZE__boundedBlockLength:
									{
										features[f++] = &feat_boundedblocklength;
										requires_bounded_block_length = true;
									}	break;
									case BUF_SIZE__fixedBlockLength:
									{
										features[f++] = &feat_fixedblocklength;
									}	break;
									case BUF_SIZE__powerOf2BlockLength:
									{
										features[f++] = &feat_powerof2blocklength;
									}	break;
									case BUF_SIZE__coarseBlockLength:
									{
										features[f++] = &feat_coarseblocklength;
									}	break;
									case STATE__loadDefaultState:
									{
										features[f++] = &feat_loaddefaultstate;
									}	break;
									case STATE__threadSafeRestore:
									{
										features[f++] = &feat_threadsaferestore;
									}	break;
									case INLINEDISPLAY__queue_draw:
									{
										features[f++] = &feat_idispqueuedraw;
									}	break;
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

						LilvNodes *required_options = lilv_plugin_get_value(app.plugin, NODE(&app, OPTIONS__requiredOption));
						if(required_options)
						{
							LILV_FOREACH(nodes, itr, required_options)
							{
								const LilvNode *option = lilv_nodes_get(required_options, itr);
								const LV2_URID opt = app.map->map(app.map->handle, lilv_node_as_uri(option));

								switch(opt)
								{
									case PARAMETERS__sampleRate:
									{
										opts[n_opts++] = opts_sampleRate;
									} break;
									case BUF_SIZE__minBlockLength:
									{
										opts[n_opts++] = opts_minBlockLength;
										requires_min_block_length = true;
									} break;
									case BUF_SIZE__maxBlockLength:
									{
										opts[n_opts++] = opts_maxBlockLength;
										requires_max_block_length = true;
									} break;
									case BUF_SIZE__nominalBlockLength:
									{
										opts[n_opts++] = opts_nominalBlockLength;
									} break;
									case BUF_SIZE__sequenceSize:
									{
										opts[n_opts++] = opts_sequenceSize;
									} break;
									case UI__updateRate:
									{
										opts[n_opts++] = opts_updateRate;
									} break;
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
					app.descriptor = app.instance
						? lilv_instance_get_descriptor(app.instance)
						: NULL;

					if(app.instance)
					{
						app.work_iface = lilv_instance_get_extension_data(app.instance, LV2_WORKER__interface);
						app.idisp_iface = lilv_instance_get_extension_data(app.instance, LV2_INLINEDISPLAY__interface);
						app.state_iface = lilv_instance_get_extension_data(app.instance, LV2_STATE__interface);
						app.opts_iface = lilv_instance_get_extension_data(app.instance, LV2_OPTIONS__interface);

						const bool has_load_default = lilv_plugin_has_feature(app.plugin,
							NODE(&app, STATE__loadDefaultState));
						if(has_load_default)
						{
							const LilvNode *pset = lilv_plugin_get_uri(app.plugin);

							lilv_world_load_resource(app.world, pset);

							LilvState *state = lilv_state_new_from_world(app.world, app.map, pset);
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

							LilvNode *minor_version_nodes = lilv_plugin_get_value(app.plugin , NODE(&app, CORE__minorVersion));
							if(minor_version_nodes)
							{
								const LilvNode *minor_version_node = lilv_nodes_get_first(minor_version_nodes);
								if(minor_version_node && lilv_node_is_int(minor_version_node))
								{
									minor_version = lilv_node_as_int(minor_version_node);
								}

								lilv_nodes_free(minor_version_nodes);
							}

							LilvNode *micro_version_nodes = lilv_plugin_get_value(app.plugin , NODE(&app, CORE__microVersion));
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
						app.descriptor = NULL;
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
		}

		char *docu = NULL;

		if(app->debug)
		{
			if(ret->dsc)
			{
				docu = lv2lint_strdup(ret->dsc);
			}
			else
			{
				LilvNode *subj_node = ret->uri ? lilv_new_uri(app->world, ret->uri) : NULL;
				if(subj_node)
				{
					LilvNode *docu_node = lilv_world_get(app->world, subj_node, NODE(app, CORE__documentation), NULL);
					if(docu_node)
					{
						docu = lv2lint_node_as_string_strdup(docu_node);

						lilv_node_free(docu_node);
					}

					lilv_node_free(subj_node);
				}
			}
		}

		const lint_t lnt = lv2lint_extract(app, ret);

		if(res->is_whitelisted)
		{
			_report_body(app, "SKIP", ANSI_COLOR_GREEN, test, ret, repl, docu);
		}
		else
		{
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
		}

		if(docu)
		{
			free(docu);
		}

		if(repl)
		{
			free(repl);
		}

		if(res->is_whitelisted)
		{
			return; // short-circuit here
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

char *
lv2lint_node_as_string_strdup(const LilvNode *node)
{
	if(!node)
	{
		return NULL;
	}

	if(!lilv_node_is_string(node))
	{
		return NULL;
	}

	const char *str = lilv_node_as_string(node);

	if (!str)
	{
		return NULL;
	}

	return lv2lint_strdup(str);
}

char *
lv2lint_node_as_uri_strdup(const LilvNode *node)
{
	if(!node)
	{
		return NULL;
	}

	if(!lilv_node_is_uri(node))
	{
		return NULL;
	}

	const char *uri = lilv_node_as_uri(node);

	return lv2lint_strdup(uri);
}

char *
lv2lint_strdup(const char *str)
{
	static const char *empty = "";

	if(!str)
	{
		str = empty;
	}

	return strdup(str);
}
