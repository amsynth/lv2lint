/*
 * SPDX-FileCopyrightText: Hanspeter Portner <dev@open-music-kontrollers.ch>
 * SPDX-License-Identifier: Artistic-2.0
 */

#ifndef _VARCHUNK_H
#define _VARCHUNK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>

/*****************************************************************************
 * API START
 *****************************************************************************/

typedef struct _varchunk_elmnt_t varchunk_elmnt_t;
typedef struct _varchunk_t varchunk_t;

struct _varchunk_elmnt_t {
	uint32_t size;
	uint32_t gap;
};

struct _varchunk_t {
  size_t size;
  size_t mask;
	size_t rsvd;
	size_t gapd;

	memory_order acquire;
	memory_order release;

  atomic_size_t head;
  atomic_size_t tail;

  uint8_t buf [] __attribute__((aligned(sizeof(varchunk_elmnt_t))));
}; 

bool
varchunk_is_lock_free(void);

size_t
varchunk_body_size(size_t minimum);

varchunk_t *
varchunk_new(size_t minimum, bool release_and_acquire);

void
varchunk_free(varchunk_t *varchunk);

void
varchunk_init(varchunk_t *varchunk, size_t body_size, bool release_and_acquire);

void *
varchunk_write_request_max(varchunk_t *varchunk, size_t minimum, size_t *maximum);

void *
varchunk_write_request(varchunk_t *varchunk, size_t minimum);

void
varchunk_write_advance(varchunk_t *varchunk, size_t written);

const void *
varchunk_read_request(varchunk_t *varchunk, size_t *toread);

void
varchunk_read_advance(varchunk_t *varchunk);

#ifdef __cplusplus
}
#endif

#endif //_VARCHUNK_H
