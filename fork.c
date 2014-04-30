#include <unistd.h>

#include "syscall.h"

/*
 * System call definitions for fork(2) and related calls.
 */

void fork_cleanup(u_long *args, u_long ret);
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

static struct scdesc vfork_desc =
{
	.sd_num = SYS_vfork,
	.sd_name = "vfork",
	.sd_nargs = 0,
	.sd_groups = SC_GROUP_FORK,
	.sd_cleanup = fork_cleanup,
};
SYSCALL_ADD(vfork_desc);

void
fork_cleanup(u_long *args __unused, u_long ret)
{

	if (ret == 0)
		_exit(0);
}
