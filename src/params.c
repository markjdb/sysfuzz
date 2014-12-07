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

#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <nv.h>
#include <stdlib.h>
#include <string.h>

#include "params.h"
#include "syscall.h"
#include "util.h"

static void	init_defaults(void);

static nvlist_t *g_params;
static nvlist_t *g_descriptions;

void
params_init(char **args)
{
	uintmax_t num;
	char *endptr, *name, *val;
	int base;
	bool flag;

	g_params = nvlist_create(NV_FLAG_IGNORE_CASE);
	g_descriptions = nvlist_create(NV_FLAG_IGNORE_CASE);
	if (g_params == NULL || g_descriptions == NULL)
		err(1, "nvlist_create failed");

	init_defaults();

	while (*args != NULL) {
		name = val = *args;
		(void)strsep(&val, "=");
		if (val == NULL)
			errx(1, "invalid name-value pair '%s'", *args);

		if (!nvlist_exists(g_params, name))
			errx(1, "non-existent option '%s'", name);
		else if (nvlist_exists_bool(g_params, name)) {
			if (strcasecmp(val, "true") != 0 &&
			    strcasecmp(val, "false") != 0)
				errx(1,
			    "invalid value '%s' for boolean option '%s'",
				    val, name);
			flag = strcasecmp(val, "true") == 0;
			nvlist_free_bool(g_params, name);
			nvlist_add_bool(g_params, name, flag);
		} else if (nvlist_exists_number(g_params, name)) {
			base = 10;
			if (strlen(val) >= 2 && strcmp(val, "0x") == 0)
				base = 16;
			errno = 0;
			num = strtoumax(val, &endptr, base);
			if (*val == '\0' || *endptr != '\0' || errno != 0)
				errx(1,
			    "invalid value '%s' for numeric option '%s'",
				    val, name);
			else if (num > UINT64_MAX)
				errx(1, "value '%s' for '%s' is out of range",
				    val, name);
			nvlist_free_number(g_params, name);
			nvlist_add_number(g_params, name, (uint64_t)num);
		} else if (nvlist_exists_string(g_params, name)) {
			nvlist_free_string(g_params, name);
			nvlist_add_string(g_params, name, val);
		} else
			errx(1, "unknown type for option '%s'", name);

		if (nvlist_error(g_params) != 0)
			errx(1, "couldn't set option '%s': %s", name,
			    strerror(nvlist_error(g_params)));
		free(*args);
		args++;
	}
}

void
params_dump()
{
	const char *name;
	void *cookie;
	int type;

	cookie = NULL;
	while ((name = nvlist_next(g_params, &type, &cookie)) != NULL) {
		printf("%s: ", name);
		switch (type) {
		case NV_TYPE_BOOL:
			printf("%s", nvlist_get_bool(g_params, name) ?
			    "true" : "false");
			break;
		case NV_TYPE_NUMBER:
			printf("%ju", nvlist_get_number(g_params, name));
			break;
		case NV_TYPE_STRING:
			printf("%s", nvlist_get_string(g_params, name));
			break;
		default:
			errx(1, "unexpected type '%d' for option '%s'", type,
			    name);
		}
		printf("\n%s\n\n", nvlist_get_string(g_descriptions, name));
	}
}

bool
param_flag(const char *name)
{

	if (!nvlist_exists_bool(g_params, name))
		errx(1, "invalid option '%s'", name);
	return (nvlist_get_string(g_params, name));
}

uint64_t
param_number(const char *name)
{

	if (!nvlist_exists_number(g_params, name))
		errx(1, "invalid option '%s'", name);
	return (nvlist_get_number(g_params, name));
}

const char *
param_string(const char *name)
{

	if (!nvlist_exists_string(g_params, name))
		errx(1, "invalid option '%s'", name);
	return (nvlist_get_string(g_params, name));
}

static void
init_defaults()
{
	char tmppath[PATH_MAX];

	snprintf(tmppath, sizeof(tmppath), "%s", "/tmp/sysfuzz.XXXXXX");
	if (mkdtemp(tmppath) == NULL)
		err(1, "mkdtemp");

	struct {
		const char *name;
		const char *descr;
		int type;
		union {
			const char *string;
			uint64_t number;
			bool flag;
		};
	} params[] = {
	{
		.name = "hier-depth",
		.descr = "Maximum file hierarchy depth.",
		.type = NV_TYPE_NUMBER,
		.number = 4,
	},
	{
		.name = "hier-max-fsize",
		.descr = "Maximum file size for random file creation.",
		.type = NV_TYPE_NUMBER,
		.number = (1024 * 1024),
	},
	{
		.name = "hier-max-files-per-dir",
		.descr = "Maximum number of random files per directory.",
		.type = NV_TYPE_NUMBER,
		.number = 10,
	},
	{
		.name = "hier-max-subdirs-per-dir",
		.descr = "Maximum number of subdirectories per directory.",
		.type = NV_TYPE_NUMBER,
		.number = 7,
	},
	{
		.name = "hier-root",
		.descr = "The root directory for a random file hierarchy.",
		.type = NV_TYPE_STRING,
		.string = tmppath,
	},
	{
		.name = "memblk-page-count",
		.descr = "The total number of pages to map in memblks.",
		.type = NV_TYPE_NUMBER,
		.number = pagecnt() / (ncpu() * 4),
	},
	{
		.name = "memblk-max-size",
		.descr = "The maximum number of pages in a memblk.",
		.type = NV_TYPE_NUMBER,
		.number = 1024,
	},
	{
		.name = "num-fuzzers",
		.descr = "The number of fuzzer processes to run.",
		.type = NV_TYPE_NUMBER,
		.number = ncpu(),
	},
	};

	for (u_int i = 0; i < nitems(params); i++) {
		switch (params[i].type) {
		case NV_TYPE_BOOL:
			nvlist_add_bool(g_params, params[i].name,
			    params[i].flag);
			break;
		case NV_TYPE_NUMBER:
			nvlist_add_number(g_params, params[i].name,
			    params[i].number);
			break;
		case NV_TYPE_STRING:
			nvlist_add_string(g_params, params[i].name,
			    params[i].string);
			break;
		default:
			errx(1, "invalid option type %d", params[i].type);
		}

		nvlist_add_string(g_descriptions, params[i].name,
		    params[i].descr);
	}
}
