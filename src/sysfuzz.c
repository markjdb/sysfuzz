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
#include <sys/syscall.h>

#include <err.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
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
sctable_alloc(char *sclist, char *scgrplist)
{
	struct sctable *table;
	struct scdesc **desc;
	const char *sc, *scgrp;
	char *list;
	size_t scs, scgrps;
	int sccnt;

	sccnt = 0;
	SET_FOREACH(desc, syscalls)
	    sccnt++;

	table = malloc(sizeof(*table) + sccnt * sizeof(struct scdesc *));
	if (table == NULL)
		err(1, "malloc");

	/* Validate the list of syscall and syscall group filters. */
	scs = scgrps = 0;
	list = sclist;
	while ((sc = strsep(&list, ",")) != NULL) {
		if (!sc_lookup(sc, NULL))
			errx(1, "unknown syscall '%s'", sc);
		scs++;
	}
	list = scgrplist;
	while ((scgrp = strsep(&list, ",")) != NULL) {
		if (!scgroup_lookup(scgrp, NULL))
			errx(1, "unknown syscall group '%s'", scgrp);
		scgrps++;
	}

	sccnt = 0;
	SET_FOREACH(desc, syscalls) {
		if (!sc_filter(*desc, sclist, scs, scgrplist, scgrps))
			table->scds[sccnt++] = *desc;
	}
	table->cnt = sccnt;

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
	gid_t gid;

	if (geteuid() != 0)
		return;

	pwd = getpwnam("nobody");
	if (pwd == NULL)
		return;
	grp = getgrnam("nobody");
	if (grp == NULL)
		return;

	gid = grp->gr_gid;
	if (setgroups(1, &gid) != 0)
		err(1, "setgroups");
	if (setgid(grp->gr_gid) != 0)
		err(1, "setgid");
	if (setuid(pwd->pw_uid) != 0)
		err(1, "setuid");
}

static void
usage()
{

	fprintf(stderr,
	    "Usage: %s [-p] -c <syscall1>[,<syscall2>,...]\n"
	    "\t-g <scgroup1>[,<scgroup2>,...]\n"
	    "\t-x <param>[=<value>]\n", getprogname());
	exit(1);
}

int
main(int argc __unused, char **argv __unused)
{
	char *sclist, *scgrplist;
	int ch, dropprivs = 1;

	sclist = scgrplist = NULL;
	while ((ch = getopt(argc, argv, "c:g:px:")) != -1)
		switch (ch) {
		case 'c':
			sclist = strdup(optarg);
			if (sclist == NULL)
				err(1, "strdup failed");
			break;
		case 'g':
			scgrplist = strdup(optarg);
			if (scgrplist == NULL)
				err(1, "strdup failed");
			break;
		case 'p':
			dropprivs = 0;
			break;
		case 'x':
		case '?':
			usage();
			break;
		}

	/*
	 * XXX there seems to be a truss/ptrace(2) bug which causes it to stop
	 * tracing when the traced process changes its uid.
	 */
	if (dropprivs)
		drop_privs();

	argpool_init();

	scloop(sctable_alloc(sclist, scgrplist));

	free(sclist);
	free(scgrplist);

	return (0);
}
