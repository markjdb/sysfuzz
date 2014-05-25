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

#include <string.h>

#include "syscall.h"

static struct {
	enum scgroup	id;
	const char	*name;
} scgroups[] = {
	{ SC_GROUP_VM, "vm" },
	{ SC_GROUP_SCHED, "sched" },
	{ SC_GROUP_FORK, "fork" },
};

/* Determine whether to fuzz the given system call. */
bool
sc_filter(const struct scdesc *desc, const char *sclist, size_t scs,
    const char *scgrplist, size_t scgrps)
{
	const char *sc, *scgrp;
	u_int grpmask;

	for (sc = sclist; scs > 0; sc += strlen(sc) + 1) {
		if (strcasecmp(sc, desc->sd_name) == 0)
			return (false);
		scs--;
	}

	grpmask = 0;
	for (scgrp = scgrplist; scgrps > 0; scgrp += strlen(scgrp) + 1) {
		(void)scgroup_lookup(scgrp, &grpmask);
		scgrps--;
	}
	if ((grpmask & desc->sd_groups) != 0)
		return (false);

	return (sclist != NULL || scgrplist != NULL);
}

/* Look up a system call by name. */
bool
sc_lookup(const char *name, int *sc)
{
	struct scdesc **_desc, *desc;

	SET_FOREACH(_desc, syscalls) {
		desc = *_desc;
		if (strcasecmp(desc->sd_name, name) == 0) {
			if (sc != NULL)
				*sc = desc->sd_num;
			return (true);
		}
	}

	return (false);
}

/* Look up a system call group by name. */
bool
scgroup_lookup(const char *name, enum scgroup *group)
{
	u_int i;

	for (i = 0; i < sizeof(scgroups) / sizeof(scgroups[0]); i++)
		if (strcasecmp(scgroups[i].name, name) == 0) {
			if (group != NULL)
				*group |= scgroups[i].id;
			return (true);
		}

	return (false);
}
