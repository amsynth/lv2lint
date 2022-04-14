<!--
  -- SPDX-FileCopyrightText: Hanspeter Portner <dev@open-music-kontrollers.ch>
  -- SPDX-License-Identifier: CC0-1.0
  -->
## lv2lint

### Check whether a given LV2 plugin is up to the specification

An LV2 lint-like tool that checks whether a given plugin and its UI(s) match up
with the provided metadata and adhere to well-known best practices.

Run it as part of your continuous integration pipeline together with
lv2/sord\_validate to reduce the likelihood of shipping plugins with major flaws
in order to prevent unsatisfied users.

*Note: This is an early release, if you happen to find false-positive warnings
when using this tool, please report back, so it can be fixed.*

#### Build status

[![build status](https://gitlab.com/OpenMusicKontrollers/lv2lint/badges/master/build.svg)](https://gitlab.com/OpenMusicKontrollers/lv2lint/commits/master)

### Binaries

#### Stable release

* [lv2lint-0.16.0.zip](https://dl.open-music-kontrollers.ch/lv2lint/stable/lv2lint-0.16.0.zip) ([sig](https://dl.open-music-kontrollers.ch/lv2lint/stable/lv2lint-0.16.0.zip.sig))

#### Unstable (nightly) release

* [lv2lint-latest-unstable.zip](https://dl.open-music-kontrollers.ch/lv2lint/unstable/lv2lint-latest-unstable.zip) ([sig](https://dl.open-music-kontrollers.ch/lv2lint/unstable/lv2lint-latest-unstable.zip.sig))

### Sources

#### Stable release

* [lv2lint-0.16.0.tar.xz](https://git.open-music-kontrollers.ch/lv2/lv2lint/snapshot/lv2lint-0.16.0.tar.xz)([sig](https://git.open-music-kontrollers.ch/lv2/lv2lint/snapshot/lv2lint-0.16.0.tar.xz.asc))

#### Git repository

* <https://git.open-music-kontrollers.ch/lv2/lv2lint>

### Packages

* [ArchLinux](https://www.archlinux.org/packages/community/x86_64/lv2lint/)

### Bugs and feature requests

* [Gitlab](https://gitlab.com/OpenMusicKontrollers/lv2lint)
* [Github](https://github.com/OpenMusicKontrollers/lv2lint)


### Dependencies

#### Mandatory

* [LV2](http://lv2plug.in/) (LV2 Plugin Standard)
* [lilv](https://drobilla.net/software/lilv/) (LV2 plugin host library)

#### Optional

* [libcurl](https://curl.haxx.se/libcurl/) (The multiprotocol file transfer library)
* [libelf](https://sourceware.org/elfutils/) (ELF object file access library)
* [libX11](https://www.xorg) (X Window System)

lv2lint can optionally test your plugin URIs for existence. If you want that,
you need to enable it at compile time (-Donline-tests=enabled) and link to libcurl.
You will also need to enable it at run-time (-o), e.g. double-opt-in.

lv2lint can optionally test your plugin symbol visibility and link dependencies.
If you want that, you need to enable it at compile time (-Delf-tests=enabled) and
link to libelf.

lv2lint can optionally test your plugin X11 UI instantiation.
If you want that, you need to enable it at compile time (-Dx11-tests=enabled) and
link to libX11.

### Build / install

	git clone https://git.open-music-kontrollers.ch/lv2/lv2lint
	cd lv2lint
	meson -Donline-tests=enabled -Delf-tests=enabled -Dx11-tests=enabled build
	cd build
	ninja
	sudo ninja install

#### Compile options

* online-tests (check URIs via libcurl, default=off)
* elf-tests (check shared object link symbols and dependencies, default=off)

### Usage

Information about the command line arguments are described in the man page:

	man 1 lv2lint

An __acceptable plugin__ *SHOULD* pass without triggering any fails, this is
also the default configuration:

	lv2lint -I ${MY_BUNDLE_DIR} http://lv2plug.in/plugins/eg-scope#Stereo

A __good plugin__ *SHOULD* pass without triggering any warnings:

	lv2lint -I ${MY_BUNDLE_DIR} -E warn http://lv2plug.in/plugins/eg-scope#Stereo

A __perfect plugin__ *SHOULD* pass without triggering any warnings or notes:

	lv2lint -I ${MY_BUNDLE_DIR} -E warn -E note http://lv2plug.in/plugins/eg-scope#Stereo

If you get any warnings or notes, you can enable debugging output to help you
fix the problems:

	lv2lint -d -I ${MY_BUNDLE_DIR} -E warn -E note http://lv2plug.in/plugins/eg-scope#Stereo

By default, lv2lint runs in packager mode and skips some tests. The latter are
important only for plugins that are distributed in binary form by the developer directly.
To activate those tests, run in (nopack)ager mode:

	lv2lint -I ${MY_BUNDLE_DIR} -M nopack http://lv2plug.in/plugins/eg-scope#Stereo

If you want to skip some tests (because you know that they fail), you can do
so by specifying patterns for tests and plugin/and or ui URI on the command line.

E.g. to skip tests about missing version information:

	lv2lint -I ${MY_BUNDLE_DIR} -t '*version*' urn:example:myplug#mono

E.g. to skip all tests that are dsp-related (and only honor ui-related ones):

	lv2lint -I ${MY_BUNDLE_DIR} -u urn:example:myplug#mono -t '*' urn:example:myplug#mono

E.g. to skip all tests that are ui-related (and only honor dsp-related ones):

	lv2lint -I ${MY_BUNDLE_DIR} -u urn:example:myplug#ui -t '*' urn:example:myplug#mono

E.g. to skip tests about extension data only on the ui:

	lv2lint -I ${MY_BUNDLE_DIR} -u urn:example:myplug#ui -t '*extension*data*' urn:example:myplug#mono

### License

SPDX-FileCopyrightText: Hanspeter Portner <dev@open-music-kontrollers.ch>
SPDX-License-Identifier: Artistic-2.0
