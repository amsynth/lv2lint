<!--
  -- (SPDX-FileCopyrightText: Hanspeter Portner <dev@open-music-kontrollers.ch>
  -- (SPDX-License-Identifier: CC0-1.0
  -->
# Varchunk

## Ringbuffer optimized for realtime event handling

### Properties

* Is realtime-safe
* Is lock-free
* Supports variably sized chunks
* Supports contiguous memory chunks
* Supports zero copy operation
* Uses a simplistic API

### Build Status

[![build status](https://gitlab.com/OpenMusicKontrollers/varchunk/badges/master/build.svg)](https://gitlab.com/OpenMusicKontrollers/varchunk/commits/master)

### Build / test

	git clone https://git.open-music-kontrollers.ch/lad/varchunk
	cd varchunk
	meson build
	cd build
	ninja -j4
	ninja test

### Dependencies

#### Optional

* [REUSE](https://git.fsfe.org/reuse/tool) (tool for compliance with the REUSE recommendations)

### Usage

	#include <pthread.h>
	#include <varchunk.h>

	static void *
	producer_main(void *arg)
	{
		varchunk_t *varchunk = arg;
		void *ptr;
		const size_t towrite = sizeof(uint32_t);
		uint32_t counter = 0;

		while(counter <= 1000000)
		{
			if( (ptr = varchunk_write_request(varchunk, towrite)) )
			{
				// write 'towrite' bytes to 'ptr'
				*(uint32_t *)ptr = counter++;
				varchunk_write_advance(varchunk, towrite);
			}
		}

		return NULL;
	}

	static void *
	consumer_main(void *arg)
	{
		varchunk_t *varchunk = arg;
		const void *ptr;
		size_t toread;

		while(1)
		{
			if( (ptr = varchunk_read_request(varchunk, &toread)) )
			{
				// read 'toread' bytes from 'ptr'
				if(*(uint32_t *)ptr >= 1000000)
					break;
				varchunk_read_advance(varchunk);
			}
		}

		return NULL;
	}

	int
	main(int argc, char **argv)
	{
		if(!varchunk_is_lock_free())
			return -1;

		pthread_t producer;
		pthread_t consumer;
		varchunk_t *varchunk = varchunk_new(8192, true);
		if(!varchunk)
			return -1;

		pthread_create(&consumer, NULL, consumer_main, varchunk);
		pthread_create(&producer, NULL, producer_main, varchunk);

		pthread_join(producer, NULL);
		pthread_join(consumer, NULL);

		varchunk_free(varchunk);

		return 0;
	}

### License

SPDX-FileCopyrightText: Hanspeter Portner <dev@open-music-kontrollers.ch>
SPDX-License-Identifier: Artistic-2.0
