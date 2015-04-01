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

#include <sched.h>

#include "syscall.h"

/*
 * System call definitions for sched_*.
 */

static struct scdesc sched_setparam_desc __unused = {
	.sd_num = SYS_sched_setparam,
	.sd_name = "sched_setparam",
	.sd_nargs = 2,
	.sd_groups = SC_GROUP_SCHED,
	.sd_args = {
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

static struct scdesc sched_getparam_desc __unused = {
	.sd_num = SYS_sched_getparam,
	.sd_name = "sched_getparam",
	.sd_nargs = 2,
	.sd_groups = SC_GROUP_SCHED,
	.sd_args = {
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

static int sched_policies[] =
{
	SCHED_FIFO,
	SCHED_OTHER,
	SCHED_RR,
};

static struct scdesc sched_setscheduler_desc __unused = {
	.sd_num = SYS_sched_setscheduler,
	.sd_name = "sched_setscheduler",
	.sd_nargs = 3,
	.sd_groups = SC_GROUP_SCHED,
	.sd_args = {
		{
			.sa_type = ARG_PID,
			.sa_name = "pid",
		},
		{
			.sa_type = ARG_CMD,
			.sa_name = "policy",
			.sa_cmds = sched_policies,
			.sa_argcnt = nitems(sched_policies),
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

static struct scdesc sched_getscheduler_desc __unused = {
	.sd_num = SYS_sched_getscheduler,
	.sd_name = "sched_getscheduler",
	.sd_nargs = 1,
	.sd_groups = SC_GROUP_SCHED,
	.sd_args = {
		{
			.sa_type = ARG_PID,
			.sa_name = "pid",
		},
	},
};
#ifdef notyet
SYSCALL_ADD(sched_getscheduler_desc);
#endif

static struct scdesc sched_yield_desc __unused = {
	.sd_num = SYS_sched_yield,
	.sd_name = "sched_yield",
	.sd_nargs = 0,
	.sd_groups = SC_GROUP_SCHED,
};
#ifdef notyet
SYSCALL_ADD(sched_yield_desc);
#endif

static struct scdesc sched_get_priority_max_desc __unused = {
	.sd_num = SYS_sched_get_priority_max,
	.sd_name = "sched_get_priority_max",
	.sd_nargs = 1,
	.sd_groups = SC_GROUP_SCHED,
	.sd_args = {
		{
			.sa_type = ARG_CMD,
			.sa_name = "policy",
			.sa_cmds = sched_policies,
			.sa_argcnt = nitems(sched_policies),
		},
	},
};
#ifdef notyet
SYSCALL_ADD(sched_get_priority_max_desc);
#endif

static struct scdesc sched_get_priority_min_desc __unused = {
	.sd_num = SYS_sched_get_priority_min,
	.sd_name = "sched_get_priority_min",
	.sd_nargs = 1,
	.sd_groups = SC_GROUP_SCHED,
	.sd_args = {
		{
			.sa_type = ARG_CMD,
			.sa_name = "policy",
			.sa_cmds = sched_policies,
			.sa_argcnt = nitems(sched_policies),
		},
	},
};
#ifdef notyet
SYSCALL_ADD(sched_get_priority_min_desc);
#endif

static struct scdesc sched_rr_get_interval_desc __unused = {
	.sd_num = SYS_sched_rr_get_interval,
	.sd_name = "sched_rr_get_interval",
	.sd_nargs = 2,
	.sd_groups = SC_GROUP_SCHED,
	.sd_args = {
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
