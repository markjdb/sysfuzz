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

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <sys/param.h>
#include <sys/linker_set.h>
#include <sys/syscall.h>

#include <stdbool.h>

SET_DECLARE(syscalls, struct scdesc);

#define	SYSCALL_ADD(desc)	DATA_SET(syscalls, desc)

enum scargtype {
	ARG_UNSPEC,
	ARG_FD,
	ARG_PATH,
	ARG_SOCKET,
	ARG_MEMADDR,
	ARG_MEMLEN,
	ARG_MODE,
	ARG_PID,
	ARG_PROCDESC,
	ARG_IFLAGMASK,
	ARG_LFLAGMASK,
	ARG_CMD,
	ARG_UID,
	ARG_GID,
	ARG_KQUEUE,
	ARG_SCHED_PARAM,
	ARG_TIMESPEC,
};

/* System call argument descriptor. */
struct scargdesc {
	enum scargtype	sa_type;	/* argument type */
	const char	*sa_name;	/* argument name */
	union {
		int	*sa_iflags;
		long	*sa_lflags;
		int	*sa_cmds;
	};
	int		sa_argcnt;
};

enum scgroup {
	SC_GROUP_VM =		(1 << 0),
	SC_GROUP_SCHED =	(1 << 1),
	SC_GROUP_FORK =		(1 << 2),
};

#define	SYSCALL_MAXARGS	8

/*
 * A system call descriptor. This contains all the static information needed
 * to test a given system call.
 */
struct scdesc {
	int		sd_num;		/* system call number */
	const char	*sd_name;	/* system call name */
	int		sd_nargs;	/* number of arguments */
	u_int		sd_groups;	/* system call groups */
	void (*sd_fixup)(u_long *);	/* pre-syscall hook */
	void (*sd_cleanup)(u_long *, u_long); /* post-syscall hook */
	struct scargdesc sd_args[];	/* argument descriptors */
};

bool	sc_filter(const struct scdesc *, const char *, size_t, const char *,
	    size_t);
bool	sc_lookup(const char *, int *);
bool	scgroup_lookup(const char *, enum scgroup *);

#endif /* _SYSCALL_H_ */
