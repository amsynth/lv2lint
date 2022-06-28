/*
 * SPDX-FileCopyrightText: Hanspeter Portner <dev@open-music-kontrollers.ch>
 * SPDX-License-Identifier: Artistic-2.0
 */

#ifndef _LV2LINT_ALLOC_H
#define _LV2LINT_ALLOC_H

#include <stdbool.h>

typedef enum _shift_t
{
	SHIFT_malloc = 0,
	SHIFT_free,
	SHIFT_calloc,
	SHIFT_realloc,
	SHIFT_posix_memalign,
	SHIFT_aligned_alloc,
	SHIFT_valloc,
	SHIFT_memalign,
	SHIFT_pvalloc,

	SHIFT_pthread_mutex_lock,
	SHIFT_pthread_mutex_unlock,
	SHIFT_pthread_mutex_timedlock,

	SHIFT_sem_wait,
	SHIFT_sem_timedwait,

	SHIFT_sleep,
	SHIFT_usleep,
	SHIFT_nanosleep,
	SHIFT_clock_nanosleep,

	SHIFT_MAX
} shift_t;

#define MASK(VAL) (1 << VAL)

typedef struct _shm_t shm_t;

struct _shm_t {
	bool enabled;
	unsigned mask;
};

shm_t *
shm_attach();

void
shm_detach();

void
shm_resume(shm_t *shm);

void
shm_enable(shm_t *shm);

void
shm_pause(shm_t *shm);

unsigned
shm_disable(shm_t *shm);

bool
shm_enabled(shm_t *shm);

#endif
