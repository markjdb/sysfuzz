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
#include <sys/sysctl.h>

#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

u_int
ncpu()
{
	size_t ncpusz;
	int ncpu;

	ncpusz = sizeof(ncpu);
	if (sysctlbyname("hw.ncpu", &ncpu, &ncpusz, NULL, 0) != 0)
		err(1, "could not read hw.ncpu");

	return (ncpu);
}

u_int
pagecnt()
{
	size_t pgcntsz;
	int pgcnt;

	pgcntsz = sizeof(pgcnt);
	if (sysctlbyname("vm.stats.vm.v_page_count", &pgcnt, &pgcntsz,
	    NULL, 0) != 0)
		err(1, "could not read vm.stats.vm.v_page_count");

	return (pgcnt);
}

char *
xstrdup(const char *str)
{
	char *ret = strdup(str);
	if (ret == NULL)
		err(1, "strdup");
	return (ret);
}
