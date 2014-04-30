#include <sched.h>

#include "syscall.h"

/*
 * System call definitions for sched_*.
 */

static struct scdesc sched_setparam_desc __unused =
{
	.sd_num = SYS_sched_setparam,
	.sd_name = "sched_setparam",
	.sd_nargs = 2,
	.sd_groups = SC_GROUP_SCHED,
	.sd_args =
	{
		{
			.sa_type = ARG_PID,
			.sa_name = "pid",
		},
		{
			.sa_type = ARG_SCHED_PARAM,
			.sa_name = "param",
		},
	},
};
#ifdef notyet
SYSCALL_ADD(sched_setparam_desc);
#endif

static struct scdesc sched_getparam_desc __unused =
{
	.sd_num = SYS_sched_getparam,
	.sd_name = "sched_getparam",
	.sd_nargs = 2,
	.sd_groups = SC_GROUP_SCHED,
	.sd_args =
	{
		{
			.sa_type = ARG_PID,
			.sa_name = "pid",
		},
		{
			.sa_type = ARG_SCHED_PARAM,
			.sa_name = "param",
		},
	},
};
#ifdef notyet
SYSCALL_ADD(sched_getparam_desc);
#endif

static struct scdesc sched_setscheduler_desc __unused =
{
	.sd_num = SYS_sched_setscheduler,
	.sd_name = "sched_setscheduler",
	.sd_nargs = 3,
	.sd_groups = SC_GROUP_SCHED,
	.sd_args =
	{
		{
			.sa_type = ARG_PID,
			.sa_name = "pid",
		},
		{
			.sa_type = ARG_CMD,
			.sa_name = "policy",
			.sa_cmds =
			{
				SCHED_FIFO,
				SCHED_OTHER,
				SCHED_RR,
			},
			.sa_argcnt = 3,
		},
		{
			.sa_type = ARG_SCHED_PARAM,
			.sa_name = "param",
		},
	},
};
#ifdef notyet
SYSCALL_ADD(sched_setscheduler_desc);
#endif

static struct scdesc sched_getscheduler_desc __unused =
{
	.sd_num = SYS_sched_getscheduler,
	.sd_name = "sched_getscheduler",
	.sd_nargs = 1,
	.sd_groups = SC_GROUP_SCHED,
	.sd_args =
	{
		{
			.sa_type = ARG_PID,
			.sa_name = "pid",
		},
	},
};
#ifdef notyet
SYSCALL_ADD(sched_getscheduler_desc);
#endif

static struct scdesc sched_yield_desc __unused =
{
	.sd_num = SYS_sched_yield,
	.sd_name = "sched_yield",
	.sd_nargs = 0,
	.sd_groups = SC_GROUP_SCHED,
};
#ifdef notyet
SYSCALL_ADD(sched_yield_desc);
#endif

static struct scdesc sched_get_priority_max_desc __unused =
{
	.sd_num = SYS_sched_get_priority_max,
	.sd_name = "sched_get_priority_max",
	.sd_nargs = 1,
	.sd_groups = SC_GROUP_SCHED,
	.sd_args =
	{
		{
			.sa_type = ARG_CMD,
			.sa_name = "policy",
			.sa_cmds =
			{
				SCHED_FIFO,
				SCHED_OTHER,
				SCHED_RR,
			},
			.sa_argcnt = 3,
		},
	},
};
#ifdef notyet
SYSCALL_ADD(sched_get_priority_max_desc);
#endif

static struct scdesc sched_get_priority_min_desc __unused =
{
	.sd_num = SYS_sched_get_priority_min,
	.sd_name = "sched_get_priority_min",
	.sd_nargs = 1,
	.sd_groups = SC_GROUP_SCHED,
	.sd_args =
	{
		{
			.sa_type = ARG_CMD,
			.sa_name = "policy",
			.sa_cmds =
			{
				SCHED_FIFO,
				SCHED_OTHER,
				SCHED_RR,
			},
			.sa_argcnt = 3,
		},
	},
};
#ifdef notyet
SYSCALL_ADD(sched_get_priority_min_desc);
#endif

static struct scdesc sched_rr_get_interval_desc __unused =
{
	.sd_num = SYS_sched_rr_get_interval,
	.sd_name = "sched_rr_get_interval",
	.sd_nargs = 2,
	.sd_groups = SC_GROUP_SCHED,
	.sd_args =
	{
		{
			.sa_type = ARG_PID,
			.sa_name = "pid",
		},
		{
			.sa_type = ARG_TIMESPEC,
			.sa_name = "interval",
		},
	},
};
#ifdef notyet
SYSCALL_ADD(sched_rr_get_interval_desc);
#endif
