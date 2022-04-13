<!--
  -- (SPDX-FileCopyrightText: Hanspeter Portner <dev@open-music-kontrollers.ch>
  -- (SPDX-License-Identifier: CC0-1.0
  -->
# mapper.lv2

## Concurrent lock-free implementation of the LV2 URID extension

### Properties

* Is lock-free
* Uses a simplistic API
* Can map a fixed size of elements only
* Has fast URI mapping with constant expected time O(1)
* Has immediate URID unmaping with O(1)
* Uses quadratic probing to counteract primary clustering
* When combined with an rt-safe memory allocator
	* Is wait-free
	* Is rt-safe

### Build Status

[![build status](https://gitlab.com/OpenMusicKontrollers/mapper.lv2/badges/master/build.svg)](https://gitlab.com/OpenMusicKontrollers/mapper.lv2/commits/master)

### Build / test

	git clone https://git.open-music-kontrollers.ch/lv2/mapper.lv2
	cd mapper.lv2
	meson build
	cd build
	ninja -j4
	ninja test

### Reference

* <http://lv2plug.in/ns/ext/urid>
* <http://preshing.com/20130605/the-worlds-simplest-lock-free-hash-table/>
* <https://en.wikipedia.org/wiki/Linear_probing>
* <https://en.wikipedia.org/wiki/MurmurHash#MurmurHash3>

### License

SPDX-FileCopyrightText: Hanspeter Portner <dev@open-music-kontrollers.ch>
SPDX-License-Identifier: Artistic-2.0
