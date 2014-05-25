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

/* Look up a system call by name. */
int
sc_lookup(const char *name, int *sc)
{
	struct scdesc **_desc, *desc;

	SET_FOREACH(_desc, syscalls) {
		desc = *_desc;
		if (strcasecmp(desc->sd_name, name) == 0) {
			*sc = desc->sd_num;
			return (1);
		}
	}

	return (0);
}

/* Look up a system call group by name. */
int
scgroup_lookup(const char *name, enum scgroup *group)
{
	u_int i;

	for (i = 0; i < sizeof(scgroups) / sizeof(scgroups[0]); i++)
		if (strcasecmp(scgroups[i].name, name) == 0) {
			*group |= scgroups[i].id;
			return (1);
		}

	return (0);
}
