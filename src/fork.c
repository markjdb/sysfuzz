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
#include <sys/wait.h>

#include <err.h>
#include <unistd.h>

#include "syscall.h"

/*
 * System call definitions for fork(2) and related calls.
 */

void rfork_fixup(u_long *args);
void fork_cleanup(u_long *args, u_long ret);

static struct scdesc fork_desc =
{
	.sd_num = SYS_fork,
	.sd_name = "fork",
	.sd_nargs = 0,
	.sd_groups = SC_GROUP_FORK,
	.sd_cleanup = fork_cleanup,
};
SYSCALL_ADD(fork_desc);

static struct scdesc rfork_desc =
{
	.sd_num = SYS_rfork,
	.sd_name = "rfork",
	.sd_nargs = 1,
	.sd_groups = SC_GROUP_FORK,
	.sd_fixup = rfork_fixup,
	.sd_cleanup = fork_cleanup,
	.sd_args =
	{
		{
			.sa_type = ARG_IFLAGMASK,
			.sa_name = "flags",
			.sa_iflags =
			{
				RFPROC,
				RFNOWAIT,
				RFFDG,
				RFCFDG,
				RFTHREAD,
				RFMEM,
				RFSIGSHARE,
				RFTSIGZMB,
				RFLINUXTHPN,
			},
		},
	},
};
SYSCALL_ADD(rfork_desc);

void
rfork_fixup(u_long *args)
{

	args[0] |= RFPROC;
	args[0] &= ~RFMEM;
}

#ifdef notyet
static struct scdesc vfork_desc =
{
	.sd_num = SYS_vfork,
	.sd_name = "vfork",
	.sd_nargs = 0,
	.sd_groups = SC_GROUP_FORK,
	.sd_cleanup = fork_cleanup,
};
SYSCALL_ADD(vfork_desc);
#endif

void
fork_cleanup(u_long *args __unused, u_long ret)
{
	int status = 0;

	if (ret == 0)
		_exit(0);
	else {
		if (wait(&status) == -1)
			err(1, "wait");
		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
			errx(1, "unexpected exit status %d\n", status);
	}
}
