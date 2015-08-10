#include <sys/param.h>

#include <assert.h>
#include <limits.h>
#include <stdlib.h>

#include "rman.h"
#include "util.h"

#define	rman_adjust(start, len) do {				\
	len += start - (start & ~(rman->rm_blksz - 1));		\
	start = start & ~(rman->rm_blksz - 1);			\
	len = roundup2(len, rman->rm_blksz);			\
} while (0)

static void	rman_validate(struct rman *rman);

int
rman_init(struct rman *rman, u_int blksz, rman_pool_init initcb)
{

	if (__bitcount(blksz) != 1)
		/* blksz must be a power of two. */
		return (1);
	TAILQ_INIT(&rman->rm_res);
	rman->rm_blksz = blksz;
	rman->rm_entries = 0;
	if (initcb != NULL)
		return (initcb(rman));
	return (0);
}

/*
 * Insert the resource range [start, start+len) into the list, coalescing
 * entries if needed. start+len must be at most ULONG_MAX; in particular, a
 * range cannot contain ULONG_MAX.
 */
void
rman_add(struct rman *rman, u_long start, u_long len)
{
	struct resource *nres, *res, *next;

	assert(ULONG_MAX - start >= len);

	if (len == 0)
		return;

	rman_adjust(start, len);

	TAILQ_FOREACH(res, &rman->rm_res, r_next) {
		if (start > res->r_start + res->r_len)
			continue;

		/* We need to add a new resource record. */
		if (start + len < res->r_start)
			break;

		/* The new range and the current range overlap. */
		res->r_start = min(start, res->r_start);
		if (start <= res->r_start)
			res->r_len += res->r_start - start;
		else
			len += start - res->r_start;
		res->r_len = max(res->r_len, len);
		assert(res->r_len > 0);

		/* Coalesce adjacent overlapping ranges. */
		while ((next = TAILQ_NEXT(res, r_next)) != NULL &&
		    next->r_start <= res->r_start + res->r_len) {
			next->r_len += next->r_start - res->r_start;
			res->r_len = max(next->r_len, res->r_len);
			TAILQ_REMOVE(&rman->rm_res, next, r_next);
			free(next);
			rman->rm_entries--;
		}
		assert(res->r_len > 0);
		goto done;
	}

	nres = xmalloc(sizeof(*nres));
	nres->r_start = start;
	nres->r_len = len;
	rman->rm_entries++;
	if (res != NULL)
		TAILQ_INSERT_BEFORE(res, nres, r_next);
	else
		TAILQ_INSERT_TAIL(&rman->rm_res, nres, r_next);

	assert(nres->r_len > 0);
done:
	rman_validate(rman);
}

/*
 * Return a random resource range from the pool without removing it. The range
 * length must be a multiple of the rman blksz and must be at most
 * blksz * maxblks if maxblks is greater than 0.
 *
 * The return value is non-zero if no resources are available.
 */
int
rman_select(struct rman *rman, u_long *start, u_long *len, u_int maxblks)
{
	struct resource *res;
	u_int blks;
	int interval;

	if (rman->rm_entries == 0) {
		*start = *len = 0;
		return (1);
	}

	interval = (random() % rman->rm_entries);
	TAILQ_FOREACH(res, &rman->rm_res, r_next) {
		if (interval-- > 0)
			continue;
		blks = res->r_len / rman->rm_blksz;

		assert(blks > 0);
		*start = (random() % blks) * rman->rm_blksz + res->r_start;
		blks -= (*start - res->r_start) / rman->rm_blksz;
		if (maxblks > 0 && blks > maxblks)
			blks = maxblks;
		*len = ((random() % blks) + 1) * rman->rm_blksz;
		break;
	}
	assert(interval == -1);
	assert(*len % rman->rm_blksz == 0);
	assert(ULONG_MAX - *start >= *len);
	return (0);
}

/*
 * Remove the specified resource range. The range must be present.
 */
void
rman_release(struct rman *rman, u_long start, u_long len)
{
	struct resource *res, *nres;

	assert(ULONG_MAX - start >= len);

	rman_adjust(start, len);

	TAILQ_FOREACH(res, &rman->rm_res, r_next) {
		assert(start >= res->r_start);
		if (start > res->r_start + res->r_len)
			continue;

		assert(res->r_len >= len);
		if (start == res->r_start ||
		    start + len == res->r_start + res->r_len) {
			/* We just need to trim an existing range. */
			if (start == res->r_start)
				res->r_start = start + len;
			res->r_len -= len;
			if (res->r_len == 0) {
				TAILQ_REMOVE(&rman->rm_res, res, r_next);
				free(res);
				rman->rm_entries--;
			}
		} else {
			/* An existing range is getting split into two. */
			nres = xmalloc(sizeof(*nres));
			nres->r_start = start + len;
			nres->r_len = res->r_len - len - (start - res->r_start);
			assert(nres->r_len > 0);
			rman->rm_entries++;
			TAILQ_INSERT_AFTER(&rman->rm_res, res, nres, r_next);

			res->r_len = start - res->r_start;
		}
		break;
	}
	assert(res != NULL);
	rman_validate(rman);
}

#ifdef INVARIANTS
/*
 * Ensure that the resource pool is well-formed.
 */
static void
rman_validate(struct rman *rman)
{
	struct resource *res, *next;
	int count;

	assert(rman->rm_entries >= 0);

	count = 0;
	TAILQ_FOREACH(res, &rman->rm_res, r_next) {
		assert(res->r_len > 0);
		if ((next = TAILQ_NEXT(res, r_next)) != NULL)
			assert(res->r_start + res->r_len < next->r_start);
		count++;
	}
	assert(count == rman->rm_entries);
}
#else
static void
rman_validate(struct rman *rman __unused)
{
}
#endif
