/*
 * SPDX-FileCopyrightText: Hanspeter Portner <dev@open-music-kontrollers.ch>
 * SPDX-License-Identifier: Artistic-2.0
 */

#ifndef _MAPPER_H
#define _MAPPER_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <lv2/urid/urid.h>

typedef struct _mapper_t mapper_t;

typedef char *(*mapper_alloc_t)(void *data, size_t size);
typedef void (*mapper_free_t)(void *data, char *uri);

bool
mapper_is_lock_free(void);

mapper_t *
mapper_new(uint32_t nitems, uint32_t nstats, const char **stats,
	mapper_alloc_t mapper_alloc_cb, mapper_free_t mapper_free_cb, void *data);

void
mapper_free(mapper_t *mapper);

uint32_t
mapper_get_usage(mapper_t *mapper);

LV2_URID_Map *
mapper_get_map(mapper_t *mapper);

LV2_URID_Unmap *
mapper_get_unmap(mapper_t *mapper);

#ifdef __cplusplus
}
#endif

#endif //_MAPPER_H
