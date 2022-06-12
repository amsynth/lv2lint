/*
 * SPDX-FileCopyrightText: Hanspeter Portner <dev@open-music-kontrollers.ch>
 * SPDX-License-Identifier: Artistic-2.0
 */

#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <pthread.h>
#include <malloc.h>

#include <lv2lint/lv2lint_shm.h>

static shm_t *shm = NULL;

static void *(*__malloc)(size_t) = NULL;
static void  (*__free)(void *) = NULL;
static void *(*__calloc)(size_t, size_t) = NULL;
static void *(*__realloc)(void *, size_t) = NULL;
static int   (*__posix_memalign)(void **, size_t, size_t) = NULL;
static void *(*__aligned_alloc)(size_t, size_t) = NULL;
static void *(*__valloc)(size_t) = NULL;
static void *(*__memalign)(size_t, size_t) = NULL;
static void *(*__pvalloc)(size_t) = NULL;

static int   (*__pthread_mutex_lock)(pthread_mutex_t *) = NULL;
static int   (*__pthread_mutex_unlock)(pthread_mutex_t *) = NULL;

typedef struct _dict_t {
	const char *name;
	void **func;
} dict_t;

#define DICT(NAME) \
	[SHIFT_ ## NAME] = { \
		.name = #NAME, \
		.func = (void **)&__ ## NAME \
	}

static dict_t dicts [SHIFT_MAX] = {
	DICT(malloc),
	DICT(free),
	DICT(calloc),
	DICT(realloc),
	DICT(posix_memalign),
	DICT(aligned_alloc),
	DICT(valloc),
	DICT(memalign),
	DICT(pvalloc),

	DICT(pthread_mutex_lock),
	DICT(pthread_mutex_unlock)
};

static void
_init(void)
{
	shm = shm_attach();
	if(!shm)
	{
		fprintf(stderr, "Error in `shm_attach`: %s\n", dlerror());
	}

	for(shift_t s = 0; s < SHIFT_MAX; s++)
	{
		dict_t *dict = &dicts[s];

		*(dict->func) = dlsym(RTLD_NEXT, dict->name);

		if(*(dict->func) == NULL)
		{
			fprintf(stderr, "Error in dlsym(RTLD_NEXT, %s): %s\n",
				dict->name, dlerror());
		}
	}
}

static void
_mask(shift_t shift)
{
	if(!shm)
	{
		_init();
	}

	if(!shm_enabled(shm))
	{
		return;
	}

	shm->mask |= MASK(shift);
}

void *
malloc(size_t size)
{
	_mask(SHIFT_malloc);

	return __malloc(size);
}

void
free(void *ptr)
{
	_mask(SHIFT_free);

	__free(ptr);
}

void *
calloc(size_t nmemb, size_t size)
{
	_mask(SHIFT_calloc);

	return __calloc(nmemb, size);
}

void *
realloc(void *ptr, size_t size)
{
	_mask(SHIFT_realloc);

	return __realloc(ptr, size);
}

int
posix_memalign(void **memptr, size_t alignment, size_t size)
{
	_mask(SHIFT_posix_memalign);

	return __posix_memalign(memptr, alignment, size);
}

void *
aligned_alloc(size_t alignment, size_t size)
{
	_mask(SHIFT_aligned_alloc);

	return __aligned_alloc(alignment, size);
}

void *
valloc(size_t size)
{
	_mask(SHIFT_valloc);

	return __valloc(size);
}

void *
memalign(size_t alignment, size_t size)
{
	_mask(SHIFT_memalign);

	return __memalign(alignment, size);
}

void *
pvalloc(size_t size)
{
	_mask(SHIFT_pvalloc);

	return __pvalloc(size);
}

/* FIXME
 * https://www.gnu.org/software/libc/manual/html_node/Replacing-malloc.html
malloc_usable_size
*/

int
pthread_mutex_lock(pthread_mutex_t *mutex)
{
	_mask(SHIFT_pthread_mutex_lock);

	return __pthread_mutex_lock(mutex);
}

int
pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	_mask(SHIFT_pthread_mutex_unlock);

	return __pthread_mutex_unlock(mutex);
}
