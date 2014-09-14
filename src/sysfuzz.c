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
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "argpool.h"
#include "params.h"
#include "syscall.h"
#include "util.h"

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
scgrp_list(const char *scgrp)
{
	struct scdesc **desc;
	enum scgroup group;

	group = 0;
	if (!scgroup_lookup(scgrp, &group))
		errx(1, "unknown syscall group '%s'", scgrp);

	SET_FOREACH(desc, syscalls) {
		if ((group & (*desc)->sd_groups) != 0)
			printf("%s\n", (*desc)->sd_name);
	}
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
scloop(u_long ncalls, u_long seed, struct sctable *table)
{
	u_long args[SYSCALL_MAXARGS], ret, sofar;
	struct scdesc *sd;
	u_int n;
	int status;

	printf("%s: seeding with %lu\n", getprogname(), seed);

	for (n = param_number("num-fuzzers"); n > 0; n--) {
		pid_t pid = fork();
		if (pid == -1)
			err(1, "fork");
		else if (pid == 0)
			break;
	}

	if (n == 0) {
		while (true)
			wait(&status);
	} else
		srandom(seed + n);

	for (sofar = 0; ncalls == 0 || sofar < ncalls; sofar++) {
		sd = table->scds[random() % table->cnt];
		memset(args, 0, sizeof(args));
		scargs_alloc(args, sd);

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

static u_long
pickseed(void)
{
	const char *path = "/dev/urandom";
	u_long seed;
	ssize_t bytes;
	int fd;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		err(1, "opening %s", path);

	bytes = read(fd, &seed, sizeof(seed));
	if (bytes < 0)
		err(1, "reading from %s", path);
	else if (bytes != sizeof(seed))
		errx(1, "short read from %s (got %zd bytes)", path, bytes);

	return (seed);
}

static void
usage()
{
	const char *pn = getprogname();

	fprintf(stderr,
	    "Usage:\t%s [-n count] [-p] [-c <syscall1>[,<syscall2>[,...]]]\n"
	    "\t    [-g <scgroup1>[,<scgroup2>[,...]]]\n"
	    "\t    [-s <seed>] [-x <param>[=<value>]]\n", pn);
	fprintf(stderr, "\t%s -d\n", pn);
	fprintf(stderr, "\t%s -l <scgroup>\n", pn);
	exit(1);
}

int
main(int argc, char **argv)
{
	char **param, **params;
	char *end, *scgrp, *sclist, *scgrplist;
	u_long ncalls, seed;
	bool dropprivs = true, dumpparams = false;
	int ch;

	params = calloc(argc + 1, sizeof(*params));
	if (params == NULL)
		err(1, "calloc");
	param = params;

	seed = pickseed();

	scgrp = sclist = scgrplist = NULL;
	while ((ch = getopt(argc, argv, "c:dg:l:n:ps:x:")) != -1)
		switch (ch) {
		case 'c':
			sclist = strdup(optarg);
			if (sclist == NULL)
				err(1, "strdup failed");
			break;
		case 'd':
			dumpparams = true;
			break;
		case 'g':
			scgrplist = strdup(optarg);
			if (scgrplist == NULL)
				err(1, "strdup failed");
			break;
		case 'l':
			scgrp = strdup(optarg);
			break;
		case 'n':
			errno = 0;
			ncalls = strtoul(optarg, &end, 10);
			if (optarg[0] == '\0' || *end != '\0' || errno != 0)
				errx(1, "invalid parameter '%s' for -n", optarg);
			break;
		case 'p':
			dropprivs = false;
			break;
		case 's':
			errno = 0;
			seed = strtoul(optarg, &end, 10);
			if (optarg[0] == '\0' || *end != '\0' || errno != 0)
				errx(1, "invalid parameter '%s' for -s", optarg);
			break;
		case 'x':
			*param++ = strdup(optarg);
			break;
		case '?':
			usage();
			break;
		}

	/* Initialize runtime parameters. */
	params_init(params);
	free(params);

	if (dumpparams) {
		if (argc != 2)
			usage();
		params_dump();
		return (0);
	}

	if (scgrp != NULL) {
		if (argc != 3)
			usage();
		scgrp_list(scgrp);
		free(scgrp);
		return (0);
	}

	/* Create input pools for system calls. */
	argpool_init();

	/*
	 * XXX there seems to be a truss/ptrace(2) bug which causes it to stop
	 * tracing when the traced process changes its uid.
	 */
	if (dropprivs)
		drop_privs();

	scloop(ncalls, seed, sctable_alloc(sclist, scgrplist));

	free(sclist);
	free(scgrplist);

	return (0);
}
