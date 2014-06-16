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

#include "options.h"
#include "syscall.h"
#include "util.h"

static void	init_defaults(void);

static nvlist_t *g_options;
static nvlist_t *g_descriptions;

void
options_init(char **args)
{
	uintmax_t num;
	char *endptr, *name, *val;
	int base;
	bool flag;

	g_options = nvlist_create(NV_FLAG_IGNORE_CASE);
	g_descriptions = nvlist_create(NV_FLAG_IGNORE_CASE);
	if (g_options == NULL || g_descriptions == NULL)
		err(1, "nvlist_create failed");

	init_defaults();

	while (*args != NULL) {
		name = val = *args;
		(void)strsep(&val, "=");
		if (val == NULL)
			errx(1, "invalid name-value pair '%s'", *args);

		if (!nvlist_exists(g_options, name))
			errx(1, "non-existent option '%s'", name);
		else if (nvlist_exists_bool(g_options, name)) {
			if (strcasecmp(val, "true") != 0 &&
			    strcasecmp(val, "false") != 0)
				errx(1,
			    "invalid value '%s' for boolean option '%s'",
				    val, name);
			flag = strcasecmp(val, "true") == 0;
			nvlist_free_bool(g_options, name);
			nvlist_add_bool(g_options, name, flag);
		} else if (nvlist_exists_number(g_options, name)) {
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
			nvlist_free_number(g_options, name);
			nvlist_add_number(g_options, name, (uint64_t)num);
		} else if (nvlist_exists_string(g_options, name)) {
			nvlist_free_string(g_options, name);
			nvlist_add_string(g_options, name, val);
		} else
			errx(1, "unknown type for option '%s'", name);

		if (nvlist_error(g_options) != 0)
			errx(1, "couldn't set option '%s': %s", name,
			    strerror(nvlist_error(g_options)));
		free(*args);
		args++;
	}
}

void
options_dump()
{
	const char *name;
	void *cookie;
	int type;

	cookie = NULL;
	while ((name = nvlist_next(g_options, &type, &cookie)) != NULL) {
		printf("%s: ", name);
		switch (type) {
		case NV_TYPE_BOOL:
			printf("%s", nvlist_get_bool(g_options, name) ?
			    "true" : "false");
			break;
		case NV_TYPE_NUMBER:
			printf("%ju", nvlist_get_number(g_options, name));
			break;
		case NV_TYPE_STRING:
			printf("%s", nvlist_get_string(g_options, name));
			break;
		default:
			errx(1, "unexpected type '%d' for option '%s'", type,
			    name);
		}
		printf("\n%s\n\n", nvlist_get_string(g_descriptions, name));
	}
}

bool
option_flag(const char *name)
{

	if (!nvlist_exists_bool(g_options, name))
		errx(1, "invalid option '%s'", name);
	return (nvlist_get_string(g_options, name));
}

uint64_t
option_number(const char *name)
{

	if (!nvlist_exists_number(g_options, name))
		errx(1, "invalid option '%s'", name);
	return (nvlist_get_number(g_options, name));
}

const char *
option_string(const char *name)
{

	if (!nvlist_exists_string(g_options, name))
		errx(1, "invalid option '%s'", name);
	return (nvlist_get_string(g_options, name));
}

static void
init_defaults()
{
	struct {
		const char *name;
		const char *description;
		int type;
		union {
			const char *string;
			uint64_t number;
			bool flag;
		};
	} options[] = {
	{
		.name = "memblk-system-ratio",
		.description =
	    "The inverse of the fraction of system memory to use for memblks.",
		.type = NV_TYPE_NUMBER,
		.number = ncpu() * 4,
	},
	{
		.name = "memblk-max-size",
		.description = "The maximum number of pages in a memblk.",
		.type = NV_TYPE_NUMBER,
		.number = 1024,
	},
	};

	for (u_int i = 0; i < nitems(options); i++) {
		switch (options[i].type) {
		case NV_TYPE_BOOL:
			nvlist_add_bool(g_options, options[i].name,
			    options[i].flag);
			break;
		case NV_TYPE_NUMBER:
			nvlist_add_number(g_options, options[i].name,
			    options[i].number);
			break;
		case NV_TYPE_STRING:
			nvlist_add_string(g_options, options[i].name,
			    options[i].string);
			break;
		default:
			errx(1, "invalid option type %d", options[i].type);
		}

		nvlist_add_string(g_descriptions, options[i].name,
		    options[i].description);
	}
}
