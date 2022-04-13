/*
 * SPDX-FileCopyrightText: Hanspeter Portner <dev@open-music-kontrollers.ch>
 * SPDX-License-Identifier: Artistic-2.0
 */

#include <stdatomic.h>
#include <stdint.h>

typedef struct _mapper_item_t mapper_item_t;

struct _mapper_item_t {
	atomic_uintptr_t val;
	uint32_t stat;
};

struct _mapper_t {
	uint32_t nitems;
	uint32_t nitems_mask;
	atomic_uint usage;

	mapper_alloc_t alloc;
	mapper_free_t free;
	void *data;

	LV2_URID_Map map;
	LV2_URID_Unmap unmap;

	uint32_t nstats;
	const char **stats;

	mapper_item_t items [];
};
