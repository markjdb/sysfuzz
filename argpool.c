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
	struct arg_memblk	*memblks;
	int			memblkcnt;
} argpool;

static void
memblk_init()
{
	void *addr;
	size_t pgcntsz, pgsizesz, len;
	u_int pgcnt, pgsize;
	int allocs;

	memset(&argpool, 0, sizeof(argpool));

	pgcntsz = sizeof(pgcnt);
	if (sysctlbyname("vm.stats.vm.v_page_count", &pgcnt, &pgcntsz, NULL,
	    0) != 0)
		err(1, "could not read vm.stats.vm.v_page_count");
	pgsizesz = sizeof(pgsize);
	if (sysctlbyname("vm.stats.vm.v_page_size", &pgsize, &pgsizesz, NULL,
	    0) != 0)
		err(1, "could not read vm.stats.vm.v_page_size");

	/*
	 * We'll map up to a quarter of the system's memory. This should become
	 * tunable.
	 */
	pgcnt /= 4;

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
		len *= pgsize;

		addr = mmap(NULL, len, PROT_READ | PROT_WRITE | PROT_EXEC,
		    MAP_ANON, -1, 0);
		if (addr == NULL)
			err(1, "mapping %zu bytes", len);
		memset(addr, 0, len);

		argpool.memblks[argpool.memblkcnt].addr = addr;
		argpool.memblks[argpool.memblkcnt++].len = len;
	}
}

struct arg_memblk *
memblk_random()
{

	return (&argpool.memblks[random() % argpool.memblkcnt]);
}

void
argpool_init()
{

	memblk_init();
}
