/*-
 * Copyright (c) 2014 Mark Johnston <markj@FreeBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/queue.h>
#include <sys/stat.h>

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "argpool.h"
#include "params.h"
#include "rman.h"
#include "util.h"

static struct rman dirfds;
static struct rman fds;
static struct rman memblks;

static void	hier_init(const char *, int);
static void	hier_extend(int, int);
static int	memblk_init(struct rman *);

static int
memblk_init(struct rman *rman __unused)
{
	void *addr;
	size_t len;
	u_int pgcnt;

	pgcnt = param_number("memblk-page-count");
	while (pgcnt > 0) {
		/*
		 * Allow up to memblk-max-size pages in a memory block, clamp to
		 * pgcnt.
		 */
		len = (random() % param_number("memblk-max-size")) + 1;
		if (len > pgcnt)
			len = pgcnt;
		pgcnt -= len;
		len *= getpagesize();

		addr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_ANON, -1, 0);
		if (addr == NULL)
			err(1, "mmap");
		if (random() % 2 == 0)
			memset(addr, 0, len);

		ap_memblk_map(addr, len);
	}
	return (0);
}

void
ap_memblk_map(void *addr, size_t len)
{

	rman_add(&memblks, (uintptr_t)addr, len);
}

/*
 * Randomly pick a memory block from the pool.
 */
int
ap_memblk_random(struct arg_memblk *memblk)
{
	u_long start, len;

	if (rman_select(&memblks, &start, &len, 0))
		return (1);
	memblk->addr = (void *)(uintptr_t)start;
	memblk->len = len;
	return (0);
}

void
ap_memblk_unmap(void *addr, size_t len)
{

	rman_release(&memblks, (u_long)(uintptr_t)addr, len);
}

/*
 * Create a random file hierarchy rooted at the specified path.
 *
 * The size of the resulting file hierarchy is bounded by four parameters:
 * hier-depth, hier-max-fsize, hier-max-files-per-dir, and
 * hier-max-subdirs-per-dir. Indeed, if we refer to these four values by
 * n, s, f, and m respectively, the bound is given by
 *
 *     f * s * (m^{n+1} - 1) / (m - 1).
 *
 * This does not include the sizes of the directories themselves, which may
 * become significant for large f and n.
 *
 * The default values give a very rough bound of 4GB. In practice, the
 * hierarchy will be somewhat smaller.
 *
 * XXX we need to have some symlinks and hard links.
 */
static void
hier_init(const char *path, int depth)
{
	struct stat sb;
	int fd;

	if (stat(path, &sb) == 0) {
		if (!S_ISDIR(sb.st_mode))
			errx(1, "path '%s' exists and isn't a directory", path);
	} else if (mkdir(path, 0777) != 0)
		err(1, "couldn't create '%s'", path);

	fd = open(path, O_DIRECTORY | O_RDONLY);
	if (fd < 0)
		err(1, "opening '%s'", path);
	hier_extend(fd, depth);
	(void)close(fd);
}

static void
hier_extend(int dirfd, int depth)
{
	char buf[16384], file[NAME_MAX];
	ssize_t nbytes;
	u_int fsize;
	int fd, numfiles;

	memset(buf, 0, sizeof(buf));

	numfiles = (random() % param_number("hier-max-files-per-dir")) + 1;

	/* Create files. */
	for (int i = 0; i < numfiles; i++) {
		randfile(file);
		fd = openat(dirfd, file, O_CREAT | O_RDWR, 0666);
		if (fd < 0)
			err(1, "opening '%s'", file);

		fsize = random() % param_number("hier-max-fsize");
		while (fsize > 0) {
			nbytes = write(fd, buf,
			    fsize > sizeof(buf) ? sizeof(buf) : fsize);
			if (nbytes < 0)
				err(1, "writing to '%s'", file);
			fsize -= nbytes;
		}
		ap_fd_add(fd);
	}

	if (depth <= 1)
		return;

	numfiles = (random() % param_number("hier-max-subdirs-per-dir")) + 1;

	/* Create subdirs. */
	for (int i = 0; i < numfiles; i++) {
		randfile(file);
		if (mkdirat(dirfd, file, 0777) != 0)
			err(1, "creating directory '%s'", file);
		fd = openat(dirfd, file, O_DIRECTORY | O_RDONLY);
		if (fd < 0)
			err(1, "opening directory '%s'", file);
		hier_extend(fd, depth - 1);
		ap_dirfd_add(fd);
	}
}

static void
descpool_add(struct rman *rman, int fd)
{

	rman_add(rman, fd, 1);
}

static void
descpool_release(struct rman *rman, int fd)
{

	rman_release(rman, fd, 1);
}

static int
descpool_select(struct rman *rman)
{
	u_long start, len;

	rman_select(rman, &start, &len, 1);
	assert(len == 1);
	return ((int)start);
}

void
ap_fd_add(int fd)
{

	descpool_add(&fds, fd);
}

void
ap_fd_close(int fd)
{

	descpool_release(&fds, fd);
}

int
ap_fd_random(void)
{

	return (descpool_select(&fds));
}

void
ap_dirfd_add(int fd)
{

	descpool_add(&dirfds, fd);
}

void
ap_dirfd_close(int fd)
{

	descpool_release(&dirfds, fd);
}

int
ap_dirfd_random(void)
{

	return (descpool_select(&dirfds));
}

/*
 * Initialize the argument pool.
 */
void
ap_init(void)
{

	(void)rman_init(&memblks, getpagesize(), memblk_init);

	(void)rman_init(&dirfds, 1, NULL);
	(void)rman_init(&fds, 1, NULL);
	hier_init(param_string("hier-root"), param_number("hier-depth"));
}
