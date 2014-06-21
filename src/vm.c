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

#include <sys/param.h>
#include <sys/mman.h>

#include <err.h>
#include <stdlib.h>
#include <unistd.h>

#include "argpool.h"
#include "syscall.h"

/*
 * System call definitions for mmap(2) and friends.
 */

void	mmap_fixup(u_long *);
void	mmap_cleanup(u_long *, u_long);
void	mincore_fixup(u_long *);
void	mincore_cleanup(u_long *, u_long);
void	munmap_cleanup(u_long *, u_long);

static int mmap_prot_flags[] =
{
	PROT_NONE,
	PROT_READ,
	PROT_WRITE,
	PROT_EXEC,
};

static int mmap_flags[] =
{
	MAP_32BIT,
	MAP_ALIGNED_SUPER,
	MAP_ANON,
	MAP_FIXED,
	MAP_HASSEMAPHORE,
	MAP_NOCORE,
	MAP_NOSYNC,
	MAP_PREFAULT_READ,
	MAP_PRIVATE,
	MAP_SHARED,
	MAP_STACK,
	MAP_EXCL,
};

static struct scdesc mmap_desc =
{
	.sd_num = SYS_mmap,
	.sd_name = "mmap",
	.sd_nargs = 6,
	.sd_groups = SC_GROUP_VM,
	.sd_fixup = mmap_fixup,
	.sd_cleanup = mmap_cleanup,
	.sd_args =
	{
		{
			.sa_type = ARG_UNSPEC,
			.sa_name = "addr",
		},
		{
			.sa_type = ARG_UNSPEC,
			.sa_name = "len",
		},
		{
			.sa_type = ARG_IFLAGMASK,
			.sa_name = "prot",
			.sa_iflags = mmap_prot_flags,
			.sa_argcnt = nitems(mmap_prot_flags),

		},
		{
			/* XXX how to handle MAP_ALIGNED(n)? */
			.sa_type = ARG_IFLAGMASK,
			.sa_name = "flags",
			.sa_iflags = mmap_flags,
			.sa_argcnt = nitems(mmap_flags),
		},
		{
			.sa_type = ARG_FD,
			.sa_name = "fd",
		},
		{
			.sa_type = ARG_UNSPEC,
			.sa_name = "offset",
		},
	},
};
SYSCALL_ADD(mmap_desc);

void
mmap_fixup(u_long *args)
{
	struct arg_memblk memblk;

	if (ap_memblk_reclaim(&memblk) == 0) {
		args[0] = (u_long)(uintptr_t)memblk.addr;
		args[1] = memblk.len;
		args[3] &= ~(MAP_ALIGNED_SUPER | MAP_STACK | MAP_HASSEMAPHORE);
		args[3] |= MAP_ANON;
		args[4] = (u_long)-1; /* XXX */
		args[5] = 0;
	}
}

void
mmap_cleanup(u_long *args, u_long ret)
{
	struct arg_memblk memblk;
	void *addr;

	addr = (void *)(uintptr_t)ret;
	if (addr == NULL && (int)args[4] == -1) {
		memblk.addr = addr;
		memblk.len = args[1];
		ap_memblk_unmap(&memblk);
	} else if (addr != (void *)(uintptr_t)args[0])
		/*
		 * If we didn't get memory at the location requested, free it.
		 */
		(void)munmap(addr, args[1]);
}

static int madvise_cmds[] =
{
	MADV_NORMAL,
	MADV_RANDOM,
	MADV_SEQUENTIAL,
	MADV_WILLNEED,
	MADV_DONTNEED,
	MADV_FREE,
	MADV_NOSYNC,
	MADV_AUTOSYNC,
	MADV_NOCORE,
	MADV_CORE,
	MADV_PROTECT,
};

static struct scdesc madvise_desc =
{
	.sd_num = SYS_madvise,
	.sd_name = "madvise",
	.sd_nargs = 3,
	.sd_groups = SC_GROUP_VM,
	.sd_args =
	{
		{
			.sa_type = ARG_MEMADDR,
			.sa_name = "addr",
		},
		{
			.sa_type = ARG_MEMLEN,
			.sa_name = "len",
		},
		{
			.sa_type = ARG_CMD,
			.sa_name = "behav",
			.sa_cmds = madvise_cmds,
			.sa_argcnt = nitems(madvise_cmds),
		},
	},
};
SYSCALL_ADD(madvise_desc);

static struct scdesc mincore_desc =
{
	.sd_num = SYS_mincore,
	.sd_name = "mincore",
	.sd_nargs = 3,
	.sd_groups = SC_GROUP_VM,
	.sd_fixup = mincore_fixup,
	.sd_cleanup = mincore_cleanup,
	.sd_args =
	{
		{
			.sa_type = ARG_MEMADDR,
			.sa_name = "addr",
		},
		{
			.sa_type = ARG_MEMLEN,
			.sa_name = "len",
		},
		{
			.sa_type = ARG_UNSPEC,
			.sa_name = "vec",
		},
	},
};
SYSCALL_ADD(mincore_desc);

void
mincore_fixup(u_long *args)
{
	void *vec;

	/* XXX this may map memory. */
	vec = malloc(args[1] / getpagesize());
	if (vec == NULL)
		err(1, "malloc");
	args[2] = (uintptr_t)vec;
}

void
mincore_cleanup(u_long *args, u_long ret __unused)
{

	free((void *)args[2]);
}

static int minherit_cmds[] =
{
	INHERIT_SHARE,
	INHERIT_NONE,
	INHERIT_COPY,
};

static struct scdesc minherit_desc =
{
	.sd_num = SYS_minherit,
	.sd_name = "minherit",
	.sd_nargs = 3,
	.sd_groups = SC_GROUP_VM,
	.sd_args =
	{
		{
			.sa_type = ARG_MEMADDR,
			.sa_name = "addr",
		},
		{
			.sa_type = ARG_MEMLEN,
			.sa_name = "len",
		},
		{
			.sa_type = ARG_CMD,
			.sa_name = "inherit",
			.sa_cmds = minherit_cmds,
			.sa_argcnt = nitems(minherit_cmds),
		},
	},
};
SYSCALL_ADD(minherit_desc);

static struct scdesc mlock_desc =
{
	.sd_num = SYS_mlock,
	.sd_name = "mlock",
	.sd_nargs = 2,
	.sd_groups = SC_GROUP_VM,
	.sd_args =
	{
		{
			.sa_type = ARG_MEMADDR,
			.sa_name = "addr",
		},
		{
			.sa_type = ARG_MEMLEN,
			.sa_name = "len",
		},
	},
};
SYSCALL_ADD(mlock_desc);

static struct scdesc mprotect_desc =
{
	.sd_num = SYS_mprotect,
	.sd_name = "mprotect",
	.sd_nargs = 3,
	.sd_groups = SC_GROUP_VM,
	.sd_args =
	{
		{
			.sa_type = ARG_MEMADDR,
			.sa_name = "addr",
		},
		{
			.sa_type = ARG_MEMLEN,
			.sa_name = "len",
		},
		{
			.sa_type = ARG_IFLAGMASK,
			.sa_name = "prot",
			.sa_iflags = mmap_prot_flags,
			.sa_argcnt = nitems(mmap_prot_flags),
		},
	},
};
SYSCALL_ADD(mprotect_desc);

static int msync_cmds[] =
{
	MS_ASYNC,
	MS_SYNC,
	MS_INVALIDATE,
};

static struct scdesc msync_desc =
{
	.sd_num = SYS_msync,
	.sd_name = "msync",
	.sd_nargs = 3,
	.sd_groups = SC_GROUP_VM,
	.sd_args =
	{
		{
			.sa_type = ARG_MEMADDR,
			.sa_name = "addr",
		},
		{
			.sa_type = ARG_MEMLEN,
			.sa_name = "len",
		},
		{
			.sa_type = ARG_CMD,
			.sa_name = "prot",
			.sa_cmds = msync_cmds,
			.sa_argcnt = nitems(msync_cmds),
		},
	},
};
SYSCALL_ADD(msync_desc);

static struct scdesc munlock_desc =
{
	.sd_num = SYS_munlock,
	.sd_name = "munlock",
	.sd_nargs = 2,
	.sd_groups = SC_GROUP_VM,
	.sd_args =
	{
		{
			.sa_type = ARG_MEMADDR,
			.sa_name = "addr",
		},
		{
			.sa_type = ARG_MEMLEN,
			.sa_name = "len",
		},
	},
};
SYSCALL_ADD(munlock_desc);

static struct scdesc munmap_desc =
{
	.sd_num = SYS_munmap,
	.sd_name = "munmap",
	.sd_nargs = 2,
	.sd_groups = SC_GROUP_VM,
	.sd_cleanup = munmap_cleanup,
	.sd_args =
	{
		{
			.sa_type = ARG_MEMADDR,
			.sa_name = "addr",
		},
		{
			.sa_type = ARG_MEMLEN,
			.sa_name = "len",
		},
	},
};
SYSCALL_ADD(munmap_desc);

void
munmap_cleanup(u_long *args, u_long ret)
{
	struct arg_memblk memblk;

	if (ret != 0)
		/* The unmap wasn't successful. */
		return;

	/* Inform the argpool layer that we've unmapped this block. */
	memblk.addr = (void *)args[0];
	memblk.len = args[1];
	(void)ap_memblk_unmap(&memblk);
}

static int mlockall_flags[] =
{
	MCL_CURRENT,
	MCL_FUTURE,
};

static struct scdesc mlockall_desc =
{
	.sd_num = SYS_mlockall,
	.sd_name = "mlockall",
	.sd_nargs = 1,
	.sd_groups = SC_GROUP_VM,
	.sd_args =
	{
		{
			.sa_type = ARG_IFLAGMASK,
			.sa_name = "flags",
			.sa_iflags = mlockall_flags,
			.sa_argcnt = nitems(mlockall_flags),
		},
	},
};
SYSCALL_ADD(mlockall_desc);

static struct scdesc munlockall_desc =
{
	.sd_num = SYS_munlockall,
	.sd_name = "munlockall",
	.sd_nargs = 0,
	.sd_groups = SC_GROUP_VM,
};
SYSCALL_ADD(munlockall_desc);
