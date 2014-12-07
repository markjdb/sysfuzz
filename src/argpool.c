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
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "argpool.h"
#include "params.h"
#include "util.h"

static struct {
	/* mmap(2)'ed blocks. */
	struct arg_memblk	*memblks;
	int			memblkcnt;
	/* Blocks unmmaped while fuzzing. */
	struct arg_memblk	*umblks;
	int			umblkcnt;
	int			umblkcntmax;
	/* File descriptors. */
	int			*fds;
	int			fdcnt;
	int			fdcntmax;
} argpool;

static void	hier_init(const char *, int);
static void	hier_extend(int, int);
static void	memblk_init();

static void
memblk_init(void)
{
	void *addr;
	size_t len;
	u_int pgcnt;
	int allocs;

	memset(&argpool, 0, sizeof(argpool));

	pgcnt = param_number("memblk-page-count");
	allocs = 32;
	argpool.memblks = malloc(sizeof(*argpool.memblks) * allocs);
	if (argpool.memblks == NULL)
		err(1, "malloc");
	while (pgcnt > 0) {
		if (argpool.memblkcnt == allocs) {
			allocs *= 2;
			argpool.memblks = realloc(argpool.memblks,
			    sizeof(*argpool.memblks) * allocs);
			if (argpool.memblks == NULL)
				err(1, "realloc");
		}

		/* Allow up to memblk-max-size pages in a memory block. */
		len = random() % param_number("memblk-max-size");
		if (len > pgcnt)
			len = pgcnt;
		pgcnt -= len;
		len *= getpagesize();

		addr = mmap(NULL, len, PROT_READ | PROT_WRITE | PROT_EXEC,
		    MAP_ANON, -1, 0);
		if (addr == NULL)
			err(1, "mapping %zu bytes", len);
		memset(addr, 0, len); /* XXX perhaps we should omit this occasionally. */

		argpool.memblks[argpool.memblkcnt].addr = addr;
		argpool.memblks[argpool.memblkcnt].len = len;
		argpool.memblkcnt++;
	}

	/* Save some space to record blocks unmapped by munmap(2). */
	argpool.umblkcntmax = allocs * 4;
	argpool.umblks = calloc(argpool.umblkcntmax, sizeof(*argpool.umblks));
	if (argpool.umblks == NULL)
		err(1, "calloc");
}

/*
 * Randomly pick a memory block from the pool.
 */
void
ap_memblk_random(struct arg_memblk *memblk)
{
	struct arg_memblk *randblk;
	size_t pages, rpages;
	u_int ps;

	ps = getpagesize();
	randblk = &argpool.memblks[random() % argpool.memblkcnt];
	pages = randblk->len / ps;
	rpages = random() % (pages + 1);

	memblk->addr = (void *)((uintptr_t)randblk->addr +
	    ps * (random() % (pages - rpages + 1)));
	memblk->len = rpages * ps;
}

/*
 * Add a record indicating that the specified block has been unmapped.
 */
int
ap_memblk_unmap(const struct arg_memblk *memblk)
{

	/* We don't have space in the table, so indicate failure. */
	if (argpool.umblkcnt == argpool.umblkcntmax - 1)
		return (1);

	argpool.umblks[argpool.umblkcnt].addr = memblk->addr;
	argpool.umblks[argpool.umblkcnt].len = memblk->len;
	argpool.umblkcnt++;
	return (0);
}

/*
 * Attempt to obtain a memblk that has been recorded as unmapped.
 * XXX pick blocks randomly rather than FILO.
 */
int
ap_memblk_reclaim(struct arg_memblk *memblk)
{

	if (argpool.umblkcnt == 0)
		return (1);

	argpool.umblkcnt--;
	memblk->addr = argpool.umblks[argpool.umblkcnt].addr;
	memblk->len = argpool.umblks[argpool.umblkcnt].len;

	return (0);
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
		ap_fd_add(fd);
	}
}

static void
fd_init(void)
{

	argpool.fdcnt = 0;
	argpool.fdcntmax = 128;
	argpool.fds = malloc(argpool.fdcntmax * sizeof(int));
	if (argpool.fds == NULL)
		err(1, "malloc");
}

/*
 * Add the specified file descriptor to the pool.
 */
void
ap_fd_add(int fd)
{

	if (argpool.fdcnt == argpool.fdcntmax) {
		argpool.fdcntmax *= 2;
		argpool.fds = realloc(argpool.fds,
		    argpool.fdcntmax * sizeof(int));
		if (argpool.fds == NULL)
			err(1, "realloc");
	}
	argpool.fds[argpool.fdcnt++] = fd;
}

/*
 * Indicate that the specified FD has been closed and is no longer valid.
 */
void
ap_fd_close(int fd __unused)
{

}

/*
 * Randomly pick an FD from the pool.
 */
int
ap_fd_random(void)
{

	if (argpool.fdcnt == 0)
		return (-1);
	return (argpool.fds[arc4random() % argpool.fdcnt]);
}

/*
 * Initialize the argument pool.
 */
void
ap_init(void)
{

	memblk_init();
	fd_init();
	hier_init(param_string("hier-root"), param_number("hier-depth"));
}
