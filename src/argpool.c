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
#include <sys/sysctl.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "argpool.h"

static void memblk_init();

static struct {
	/* Blocks mmap(2)ed during initialization. */
	struct arg_memblk	*memblks;
	int			memblkcnt;
	/* Blocks unmmaped while fuzzing. */
	struct arg_memblk	*umblks;
	int			umblkcnt;
	int			umblkcntmax;
} argpool;

static u_int _pagesize;
static int _ncpu;

static void
memblk_init()
{
	void *addr;
	size_t pgcntsz, len;
	u_int pgcnt;
	int allocs;

	memset(&argpool, 0, sizeof(argpool));

	pgcntsz = sizeof(pgcnt);
	if (sysctlbyname("vm.stats.vm.v_page_count", &pgcnt, &pgcntsz, NULL,
	    0) != 0)
		err(1, "could not read vm.stats.vm.v_page_count");

	/* This should become tunable. */
	pgcnt /= ncpu() * 4;

	allocs = 32;
	argpool.memblks = malloc(sizeof(*argpool.memblks) * allocs);
	if (argpool.memblks == NULL)
		err(1, "malloc");
	while (pgcnt > 0) {
		if (argpool.memblkcnt == allocs) {
			allocs *= 2;
			argpool.memblks = realloc(argpool.memblks,
			    sizeof(*argpool.memblks) * allocs);
		}

		/* Allow up to 1024 pages in a memory block. */
		len = (random() % 1024);
		if (len > pgcnt)
			len = pgcnt;
		pgcnt -= len;
		len *= pagesize();

		addr = mmap(NULL, len, PROT_READ | PROT_WRITE | PROT_EXEC,
		    MAP_ANON, -1, 0);
		if (addr == NULL)
			err(1, "mapping %zu bytes", len);
		memset(addr, 0, len); /* XXX perhaps we should omit this occasionally. */

		argpool.memblks[argpool.memblkcnt].addr = addr;
		argpool.memblks[argpool.memblkcnt++].len = len;
	}

	/* Save some space to record blocks unmapped by munmap(2). */
	argpool.umblkcntmax = allocs * 4;
	argpool.umblks = calloc(argpool.umblkcntmax, sizeof(*argpool.umblks));
	if (argpool.umblks == NULL)
		err(1, "calloc");
}

void
memblk_random(struct arg_memblk *memblk)
{
	struct arg_memblk *randblk;
	size_t pages, rpages;
	u_int ps;

	ps = pagesize();
	randblk = &argpool.memblks[random() % argpool.memblkcnt];
	pages = randblk->len / ps;
	rpages = random() % pages;

	memblk->addr = (void *)((uintptr_t)randblk->addr +
	    ps * (random() % (pages - rpages)));
	memblk->len = rpages * ps;
}

int
unmapblk(struct arg_memblk *memblk)
{

	if (argpool.umblkcnt == argpool.umblkcntmax)
		return (1);

	argpool.umblks[argpool.umblkcnt].addr = memblk->addr;
	argpool.umblks[argpool.umblkcnt++].len = memblk->len;
	return (0);
}

u_int
pagesize()
{

	return (_pagesize);
}

u_int
ncpu()
{

	return (_ncpu);
}

void
argpool_init()
{
	size_t pagesizesz, ncpusz;

	pagesizesz = sizeof(_pagesize);
	if (sysctlbyname("vm.stats.vm.v_page_size", &_pagesize, &pagesizesz,
	    NULL, 0) != 0)
		err(1, "could not read vm.stats.vm.v_page_size");

	ncpusz = sizeof(_ncpu);
	if (sysctlbyname("hw.ncpu", &_ncpu, &ncpusz, NULL, 0) != 0)
		err(1, "could not read hw.ncpu");

	memblk_init();
}
