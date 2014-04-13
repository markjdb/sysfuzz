#include <sys/types.h>
#include <sys/syscall.h>

#include <err.h>
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "argpool.h"
#include "syscall.h"

struct sctable {
	int		cnt;
	struct scdesc	*scds[1];
};

static struct sctable *
sctable_alloc()
{
	struct sctable *table;
	struct scdesc **desc;
	int sccnt;

	sccnt = 0;
	SET_FOREACH(desc, syscalls)
	    sccnt++;

	table = malloc(sizeof(*table) + sccnt * sizeof(struct scdesc *));
	if (table == NULL)
		err(1, "malloc");
	table->cnt = sccnt;

	sccnt = 0;
	SET_FOREACH(desc, syscalls)
		table->scds[sccnt++] = *desc;

	return (table);
}

static void
scargs_alloc(u_long *args, struct scdesc *sd)
{
	struct arg_memblk memblk;
	int argcnt, ci;

	for (int i = 0; i < sd->sd_nargs; i++) {
		switch (sd->sd_args[i].sa_type) {
		case ARG_UNSPEC:
			args[i] = random();
			break;
		case ARG_MEMADDR:
			memblk_random(&memblk);
			args[i] = (uintptr_t)memblk.addr;
			if (i + 1 < sd->sd_nargs &&
			    sd->sd_args[i + 1].sa_type == ARG_MEMLEN)
				args[++i] = memblk.len;
			break;
		case ARG_MEMLEN:
			memblk_random(&memblk);
			args[i] = memblk.len;
			break;
		case ARG_CMD:
			ci = random() % sd->sd_args[i].sa_argcnt;
			args[i] = sd->sd_args[i].sa_cmds[ci];
			break;
		case ARG_IFLAGMASK:
			argcnt = sd->sd_args[i].sa_argcnt;
			for (int fi = random() % (argcnt + 1); fi > 0; fi--)
				args[i] |=
				    sd->sd_args[i].sa_iflags[random() % argcnt];
			break;
		default:
			args[i] = 0;
			break;
		}
	}
}

static void
scloop(struct sctable *table)
{
	u_long args[SYSCALL_MAXARGS];
	struct scdesc *sd;
	u_long ret;

	fork();

	/* XXX need to reseed. */

	while (1) {
		sd = table->scds[random() % table->cnt];
		memset(args, 0, sizeof(args));
		scargs_alloc(args, sd);

		if (sd->sd_num == SYS_mmap)
			continue; /* XXX */

		if (sd->sd_fixup != NULL)
			(sd->sd_fixup)(args);
		ret = __syscall(sd->sd_num, args[0], args[1], args[2], args[3],
		    args[4], args[5], args[6], args[7]);
		if (sd->sd_cleanup != NULL)
			(sd->sd_cleanup)(args, ret);
	}
}

static void
drop_privs()
{
	struct passwd *pwd;
	struct group *grp;

	return;
	if (geteuid() != 0)
		return;

	pwd = getpwnam("nobody");
	if (pwd == NULL)
		return;
	grp = getgrnam("nobody");
	if (grp == NULL)
		return;

	if (setgid(grp->gr_gid) != 0)
		err(1, "setgid");
	else if (setuid(pwd->pw_uid) != 0)
		err(1, "setuid");
}

int
main(int argc __unused, char **argv __unused)
{

	drop_privs();

	argpool_init();

	scloop(sctable_alloc());

	return (0);
}
