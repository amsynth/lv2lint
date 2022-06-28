/*
 * SPDX-FileCopyrightText: Hanspeter Portner <dev@open-music-kontrollers.ch>
 * SPDX-License-Identifier: Artistic-2.0
 */

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <lv2lint/lv2lint_shm.h>

shm_t *
shm_attach()
{
	shm_t *shm = NULL;
	char name [32];

	snprintf(name, sizeof(name), "/lv2lint.%d", getpid());

	int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);

	if(fd == -1)
	{
		fd = shm_open(name, O_RDWR, S_IRUSR | S_IWUSR);
	}

	if(fd == -1)
	{
		return NULL;
	}

	const size_t total_size = sizeof(shm_t);

	if(  (ftruncate(fd, total_size) == -1)
		|| ((shm = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
					MAP_SHARED, fd, 0)) == MAP_FAILED) )
	{
		close(fd);
		return NULL;
	}

	close(fd);

	shm->enabled = false;
	shm->mask = 0;

	return shm;
}

void
shm_detach()
{
	char name [32];

	snprintf(name, sizeof(name), "/lv2lint.%d", getpid());

	shm_unlink(name);
}

void
shm_resume(shm_t *shm)
{
	shm->enabled = true;
}

void
shm_enable(shm_t *shm)
{
	shm_resume(shm);
	shm->mask = 0;
}

void
shm_pause(shm_t *shm)
{
	shm->enabled = false;
}

unsigned
shm_disable(shm_t *shm)
{
	shm_pause(shm);
	return shm->mask;
}

bool
shm_enabled(shm_t *shm)
{
	return shm->enabled;
}
