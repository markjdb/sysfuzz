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

#include <sys/types.h>
#include <sys/mman.h>

#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "argpool.h"
#include "params.h"
#include "util.h"

static struct {
	/* Blocks mmap(2)ed during initialization. */
	struct arg_memblk	*memblks;
	int			memblkcnt;
	/* Blocks unmmaped while fuzzing. */
	struct arg_memblk	*umblks;
	int			umblkcnt;
	int			umblkcntmax;
} argpool;

static void
memblk_init()
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

	memblk->addr = argpool.umblks[argpool.umblkcnt].addr;
	memblk->len = argpool.umblks[argpool.umblkcnt].len;
	argpool.umblkcnt--;
	return (0);
}

void
ap_init()
{

	memblk_init();
}
