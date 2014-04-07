#include <sys/mman.h>
#include <err.h>
#include <stdlib.h>

#include "syscall.h"

/*
 * System call definitions for mmap(2) and friends.
 */

void mincore_fixup(u_long *args);
void mincore_cleanup(u_long *args, u_long ret);

static struct scdesc mmap_desc =
{
	.sd_num = SYS_mmap,
	.sd_name = "mmap",
	.sd_nargs = 6,
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
			.sa_iflags =
			{
				PROT_NONE,
				PROT_READ,
				PROT_WRITE,
				PROT_EXEC
			},
			.sa_argcnt = 4,

		},
		{
			/* XXX how to handle MAP_ALIGNED(n)? */
			.sa_type = ARG_IFLAGMASK,
			.sa_name = "flags",
			.sa_iflags =
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
			},
			.sa_argcnt = 11,
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

static struct scdesc madvise_desc =
{
	.sd_num = SYS_madvise,
	.sd_name = "madvise",
	.sd_nargs = 3,
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
			.sa_cmds =
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
			},
			.sa_argcnt = 11,
		},
	},
};
SYSCALL_ADD(madvise_desc);

static struct scdesc mincore_desc =
{
	.sd_num = SYS_mincore,
	.sd_name = "mincore",
	.sd_nargs = 3,
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

	vec = malloc(args[1] / 4096 /* XXX page size */);
	if (vec == NULL)
		err(1, "malloc");
	args[2] = (uintptr_t)vec;
}

void
mincore_cleanup(u_long *args, u_long ret __unused)
{

	free((void *)args[2]);
}

static struct scdesc minherit_desc =
{
	.sd_num = SYS_minherit,
	.sd_name = "minherit",
	.sd_nargs = 3,
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
			.sa_cmds =
			{
				INHERIT_SHARE,
				INHERIT_NONE,
				INHERIT_COPY,
			},
			.sa_argcnt = 3,
		},
	},
};
SYSCALL_ADD(minherit_desc);

static struct scdesc mlock_desc =
{
	.sd_num = SYS_mlock,
	.sd_name = "mlock",
	.sd_nargs = 2,
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
			.sa_cmds =
			{
				PROT_NONE,
				PROT_READ,
				PROT_WRITE,
				PROT_EXEC,
			},
			.sa_argcnt = 4,
		},
	},
};
SYSCALL_ADD(mprotect_desc);

static struct scdesc msync_desc =
{
	.sd_num = SYS_msync,
	.sd_name = "msync",
	.sd_nargs = 3,
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
			.sa_cmds =
			{
				MS_ASYNC,
				MS_SYNC,
				MS_INVALIDATE,
			},
			.sa_argcnt = 3,
		},
	},
};
SYSCALL_ADD(msync_desc);

static struct scdesc munlock_desc =
{
	.sd_num = SYS_munlock,
	.sd_name = "munlock",
	.sd_nargs = 2,
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
