/*
 * SPDX-FileCopyrightText: Hanspeter Portner <dev@open-music-kontrollers.ch>
 * SPDX-License-Identifier: Artistic-2.0
 */

#ifndef _LV2LINT_H
#define _LV2LINT_H

#include <unistd.h> // isatty
#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>

#include <lv2lint/lv2lint_shm.h>
#include <lv2lint/lv2lint_syscall.h>

#include <lilv/lilv.h>

#include <varchunk/varchunk.h>

#include <lv2/worker/worker.h>
#include <lv2/state/state.h>
#include <lv2/options/options.h>
#include <lv2/ui/ui.h>
#include <lv2/atom/atom.h>
#include <lv2/midi/midi.h>
#include <lv2/time/time.h>
#include <lv2/presets/presets.h>
#include <lv2/uri-map/uri-map.h>

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

#ifndef LV2_UI__makeSONameResident
#	define LV2_UI__makeSONameResident          LV2_UI_PREFIX "makeSONameResident"
#endif

#define NODE(APP, ID) (APP)->nodes[ID]

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

typedef union _port_t port_t;
typedef union _var_t var_t;
typedef struct _white_t white_t;
typedef struct _urid_t urid_t;
typedef struct _app_t app_t;
typedef struct _test_t test_t;
typedef struct _ret_t ret_t;
typedef struct _dst_t dst_t;
typedef struct _res_t res_t;
typedef const ret_t *(*test_cb_t)(app_t *app);
typedef int (*wrap_t)(app_t *app, void *data);
typedef int (*parent_t)(app_t *, pid_t);
typedef int (*child_t)(void *);

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

struct _dst_t {
	int idx;
	void *body;
};

struct _res_t {
	const ret_t *ret;
	char *urn;
	bool is_whitelisted;
};

#define PORT_NSAMPLES 32 //FIXME

union _port_t {
	float wav [PORT_NSAMPLES];
	struct {
		LV2_Atom atom;
		LV2_Atom_Sequence_Body body;
		uint8_t data [];
	} seq;
};

union _var_t {
	uint32_t u32;
	int32_t i32;
	int64_t i64;
	float f32;
	double f64;
};

typedef enum _stat_urid_t {
	STAT_URID_INVALID = 0,

	RDFS__label,
	RDFS__comment,
	RDFS__range,
	RDFS__subClassOf,

	RDF__type,
	RDF__value,

	DOAP__description,
	DOAP__license,
	DOAP__name,
	DOAP__shortdesc,

	XSD__int,
	XSD__nonNegativeInteger,
	XSD__long,
	XSD__float,
	XSD__double,

	ATOM__Atom,
	ATOM__AtomPort,
	ATOM__Blank,
	ATOM__Bool,
	ATOM__Chunk,
	ATOM__Double,
	ATOM__Event,
	ATOM__Float,
	ATOM__Int,
	ATOM__Literal,
	ATOM__Long,
	ATOM__Number,
	ATOM__Object,
	ATOM__Path,
	ATOM__Property,
	ATOM__Resource,
	ATOM__Sequence,
	ATOM__Sound,
	ATOM__String,
	ATOM__Tuple,
	ATOM__URI,
	ATOM__URID,
	ATOM__Vector,
	ATOM__atomTransfer,
	ATOM__beatTime,
	ATOM__bufferType,
	ATOM__childType,
	ATOM__eventTransfer,
	ATOM__frameTime,
	ATOM__supports,
	ATOM__timeUnit,

	BUF_SIZE__boundedBlockLength,
	BUF_SIZE__coarseBlockLength,
	BUF_SIZE__fixedBlockLength,
	BUF_SIZE__maxBlockLength,
	BUF_SIZE__minBlockLength,
	BUF_SIZE__nominalBlockLength,
	BUF_SIZE__powerOf2BlockLength,
	BUF_SIZE__sequenceSize,

	CORE__AllpassPlugin,
	CORE__AmplifierPlugin,
	CORE__AnalyserPlugin,
	CORE__AudioPort,
	CORE__BandpassPlugin,
	CORE__CVPort,
	CORE__ChorusPlugin,
	CORE__CombPlugin,
	CORE__CompressorPlugin,
	CORE__ConstantPlugin,
	CORE__ControlPort,
	CORE__ConverterPlugin,
	CORE__DelayPlugin,
	CORE__DistortionPlugin,
	CORE__DynamicsPlugin,
	CORE__EQPlugin,
	CORE__EnvelopePlugin,
	CORE__ExpanderPlugin,
	CORE__ExtensionData,
	CORE__Feature,
	CORE__FilterPlugin,
	CORE__FlangerPlugin,
	CORE__FunctionPlugin,
	CORE__GatePlugin,
	CORE__GeneratorPlugin,
	CORE__HighpassPlugin,
	CORE__InputPort,
	CORE__InstrumentPlugin,
	CORE__LimiterPlugin,
	CORE__LowpassPlugin,
	CORE__MixerPlugin,
	CORE__ModulatorPlugin,
	CORE__MultiEQPlugin,
	CORE__OscillatorPlugin,
	CORE__OutputPort,
	CORE__ParaEQPlugin,
	CORE__PhaserPlugin,
	CORE__PitchPlugin,
	CORE__Plugin,
	CORE__PluginBase,
	CORE__Point,
	CORE__Port,
	CORE__PortProperty,
	CORE__Resource,
	CORE__ReverbPlugin,
	CORE__ScalePoint,
	CORE__SimulatorPlugin,
	CORE__SpatialPlugin,
	CORE__Specification,
	CORE__SpectralPlugin,
	CORE__UtilityPlugin,
	CORE__WaveshaperPlugin,
	CORE__appliesTo,
	CORE__binary,
	CORE__connectionOptional,
	CORE__control,
	CORE__default,
	CORE__designation,
	CORE__documentation,
	CORE__enumeration,
	CORE__extensionData,
	CORE__freeWheeling,
	CORE__hardRTCapable,
	CORE__inPlaceBroken,
	CORE__index,
	CORE__integer,
	CORE__isLive,
	CORE__latency,
	CORE__maximum,
	CORE__microVersion,
	CORE__minimum,
	CORE__minorVersion,
	CORE__name,
	CORE__optionalFeature,
	CORE__port,
	CORE__portProperty,
	CORE__project,
	CORE__prototype,
	CORE__reportsLatency,
	CORE__requiredFeature,
	CORE__sampleRate,
	CORE__scalePoint,
	CORE__symbol,
	CORE__toggled,

	DATA_ACCESS,

	DYN_MANIFEST,

	EVENT__Event,
	EVENT__EventPort,
	EVENT__FrameStamp,
	EVENT__TimeStamp,
	EVENT__generatesTimeStamp,
	EVENT__generic,
	EVENT__inheritsEvent,
	EVENT__inheritsTimeStamp,
	EVENT__supportsEvent,
	EVENT__supportsTimeStamp,

	INSTANCE_ACCESS,

	LOG__Entry,
	LOG__Error,
	LOG__Note,
	LOG__Trace,
	LOG__Warning,
	LOG__log,

	MIDI__ActiveSense,
	MIDI__Aftertouch,
	MIDI__Bender,
	MIDI__ChannelPressure,
	MIDI__Chunk,
	MIDI__Clock,
	MIDI__Continue,
	MIDI__Controller,
	MIDI__MidiEvent,
	MIDI__NoteOff,
	MIDI__NoteOn,
	MIDI__ProgramChange,
	MIDI__QuarterFrame,
	MIDI__Reset,
	MIDI__SongPosition,
	MIDI__SongSelect,
	MIDI__Start,
	MIDI__Stop,
	MIDI__SystemCommon,
	MIDI__SystemExclusive,
	MIDI__SystemMessage,
	MIDI__SystemRealtime,
	MIDI__Tick,
	MIDI__TuneRequest,
	MIDI__VoiceMessage,
	MIDI__benderValue,
	MIDI__binding,
	MIDI__byteNumber,
	MIDI__channel,
	MIDI__chunk,
	MIDI__controllerNumber,
	MIDI__controllerValue,
	MIDI__noteNumber,
	MIDI__pressure,
	MIDI__programNumber,
	MIDI__property,
	MIDI__songNumber,
	MIDI__songPosition,
	MIDI__status,
	MIDI__statusMask,
	MIDI__velocity,

	MORPH__AutoMorphPort,
	MORPH__MorphPort,
	MORPH__interface,
	MORPH__supportsType,
	MORPH__currentType,

	OPTIONS__Option,
	OPTIONS__interface,
	OPTIONS__options,
	OPTIONS__requiredOption,
	OPTIONS__supportedOption,

	PARAMETERS__CompressorControls,
	PARAMETERS__ControlGroup,
	PARAMETERS__EnvelopeControls,
	PARAMETERS__FilterControls,
	PARAMETERS__OscillatorControls,
	PARAMETERS__amplitude,
	PARAMETERS__attack,
	PARAMETERS__bypass,
	PARAMETERS__cutoffFrequency,
	PARAMETERS__decay,
	PARAMETERS__delay,
	PARAMETERS__dryLevel,
	PARAMETERS__frequency,
	PARAMETERS__gain,
	PARAMETERS__hold,
	PARAMETERS__pulseWidth,
	PARAMETERS__ratio,
	PARAMETERS__release,
	PARAMETERS__resonance,
	PARAMETERS__sampleRate,
	PARAMETERS__sustain,
	PARAMETERS__threshold,
	PARAMETERS__waveform,
	PARAMETERS__wetDryRatio,
	PARAMETERS__wetLevel,

	PATCH__Ack,
	PATCH__Delete,
	PATCH__Copy,
	PATCH__Error,
	PATCH__Get,
	PATCH__Message,
	PATCH__Move,
	PATCH__Patch,
	PATCH__Post,
	PATCH__Put,
	PATCH__Request,
	PATCH__Response,
	PATCH__Set,
	PATCH__accept,
	PATCH__add,
	PATCH__body,
	PATCH__context,
	PATCH__destination,
	PATCH__property,
	PATCH__readable,
	PATCH__remove,
	PATCH__request,
	PATCH__subject,
	PATCH__sequenceNumber,
	PATCH__value,
	PATCH__wildcard,
	PATCH__writable,

	PORT_GROUPS__DiscreteGroup,
	PORT_GROUPS__Element,
	PORT_GROUPS__FivePointOneGroup,
	PORT_GROUPS__FivePointZeroGroup,
	PORT_GROUPS__FourPointZeroGroup,
	PORT_GROUPS__Group,
	PORT_GROUPS__InputGroup,
	PORT_GROUPS__MidSideGroup,
	PORT_GROUPS__MonoGroup,
	PORT_GROUPS__OutputGroup,
	PORT_GROUPS__SevenPointOneGroup,
	PORT_GROUPS__SevenPointOneWideGroup,
	PORT_GROUPS__SixPointOneGroup,
	PORT_GROUPS__StereoGroup,
	PORT_GROUPS__ThreePointZeroGroup,
	PORT_GROUPS__center,
	PORT_GROUPS__centerLeft,
	PORT_GROUPS__centerRight,
	PORT_GROUPS__element,
	PORT_GROUPS__group,
	PORT_GROUPS__left,
	PORT_GROUPS__lowFrequencyEffects,
	PORT_GROUPS__mainInput,
	PORT_GROUPS__mainOutput,
	PORT_GROUPS__rearCenter,
	PORT_GROUPS__rearLeft,
	PORT_GROUPS__rearRight,
	PORT_GROUPS__right,
	PORT_GROUPS__side,
	PORT_GROUPS__sideChainOf,
	PORT_GROUPS__sideLeft,
	PORT_GROUPS__sideRight,
	PORT_GROUPS__source,
	PORT_GROUPS__subGroupOf,

	PORT_PROPS__causesArtifacts,
	PORT_PROPS__continuousCV,
	PORT_PROPS__discreteCV,
	PORT_PROPS__displayPriority,
	PORT_PROPS__expensive,
	PORT_PROPS__hasStrictBounds,
	PORT_PROPS__logarithmic,
	PORT_PROPS__notAutomatic,
	PORT_PROPS__notOnGUI,
	PORT_PROPS__rangeSteps,
	PORT_PROPS__supportsStrictBounds,
	PORT_PROPS__trigger,

	PRESETS__Bank,
	PRESETS__Preset,
	PRESETS__bank,
	PRESETS__preset,
	PRESETS__value,

	RESIZE_PORT__asLargeAs,
	RESIZE_PORT__minimumSize,
	RESIZE_PORT__resize,

	STATE__State,
	STATE__interface,
	STATE__loadDefaultState,
	STATE__freePath,
	STATE__makePath,
	STATE__mapPath,
	STATE__state,
	STATE__threadSafeRestore,
	STATE__StateChanged,

	TIME__Time,
	TIME__Position,
	TIME__Rate,
	TIME__position,
	TIME__barBeat,
	TIME__bar,
	TIME__beat,
	TIME__beatUnit,
	TIME__beatsPerBar,
	TIME__beatsPerMinute,
	TIME__frame,
	TIME__framesPerSecond,
	TIME__speed,

	UI__CocoaUI,
	UI__Gtk3UI,
	UI__GtkUI,
	UI__PortNotification,
	UI__PortProtocol,
	UI__Qt4UI,
	UI__Qt5UI,
	UI__UI,
	UI__WindowsUI,
	UI__X11UI,
	UI__binary,
	UI__fixedSize,
	UI__idleInterface,
	UI__noUserResize,
	UI__notifyType,
	UI__parent,
	UI__plugin,
	UI__portIndex,
	UI__portMap,
	UI__portNotification,
	UI__portSubscribe,
	UI__protocol,
	UI__requestValue,
	UI__floatProtocol,
	UI__peakProtocol,
	UI__resize,
	UI__showInterface,
	UI__touch,
	UI__ui,
	UI__updateRate,
	UI__windowTitle,
	UI__scaleFactor,
	UI__foregroundColor,
	UI__backgroundColor,
	UI__makeSONameResident,

	UNITS__Conversion,
	UNITS__Unit,
	UNITS__bar,
	UNITS__beat,
	UNITS__bpm,
	UNITS__cent,
	UNITS__cm,
	UNITS__coef,
	UNITS__conversion,
	UNITS__db,
	UNITS__degree,
	UNITS__frame,
	UNITS__hz,
	UNITS__inch,
	UNITS__khz,
	UNITS__km,
	UNITS__m,
	UNITS__mhz,
	UNITS__midiNote,
	UNITS__mile,
	UNITS__min,
	UNITS__mm,
	UNITS__ms,
	UNITS__name,
	UNITS__oct,
	UNITS__pc,
	UNITS__prefixConversion,
	UNITS__render,
	UNITS__s,
	UNITS__semitone12TET,
	UNITS__symbol,
	UNITS__unit,

	URID__map,
	URID__unmap,

	URI_MAP,

	WORKER__interface,
	WORKER__schedule,

	EXTERNAL_UI__Widget,

	INLINEDISPLAY__interface,
	INLINEDISPLAY__queue_draw,

	STAT_URID_MAX
} stat_urid_t;

struct _app_t {
	LilvWorld *world;
	const char *plugin_uri;
	const LilvPlugin *plugin;
	LilvInstance *instance;
	const LV2_Descriptor *descriptor;
	const LV2UI_Descriptor *ui_descriptor;
	const LilvPort *port;
	const LilvNode *parameter;
	const LilvUI *ui;
	const char *ui_uri;
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	void *ui_instance;
	uintptr_t ui_widget;
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
	shm_t *shm;
	struct {
		unsigned connect_port;
		unsigned run;
		unsigned work_response;
	} forbidden;
	struct {
		int instantiate;
		int connect_port;
		int activate;
		int run;
		int deactivate;
		int cleanup;
		int ui_instantiate;
		int ui_cleanup;
		int work;
		int work_response;
		int state_restore;
	} status;
	varchunk_t *to_worker;
	varchunk_t *from_worker;
	bool syscall [SYSCALL_MAX];
	LilvNode *nodes [STAT_URID_MAX];
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

#ifdef ENABLE_X11_TESTS
void
test_x11(app_t *app, bool *flag);
#endif

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

char *
lv2lint_node_as_string_strdup(const LilvNode *node);

char *
lv2lint_node_as_uri_strdup(const LilvNode *node);

char *
lv2lint_strdup(const char *str);

int
log_vprintf(void *data, LV2_URID type , const char *fmt, va_list args);

int
log_printf(void *data, LV2_URID type, const char *fmt, ...);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
uint32_t
uri_to_id(LV2_URI_Map_Callback_Data instance, const char *_map, const char *uri);
#pragma GCC diagnostic pop

void
lv2lint_append_to(char **dst, const char *src);

int
lv2lint_wrap(app_t *app, wrap_t wrap, void *data);

#endif
