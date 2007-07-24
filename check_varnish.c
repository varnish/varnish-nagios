/*-
 * Copyright (c) 2007 Linpro AS
 * All rights reserved.
 *
 * Author: Cecilie Fritzvold <cecilihf@linpro.no>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 *
 * Nagios plugin for Varnish
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "shmlog.h"
#include "varnishapi.h"



/*
 * Check if the tresholds against the value and return the appropriate
 * status code.
 */
static int
check_treshold(intmax_t value, int warn, int crit, int less)
{

	if (!less) {
		if (value < warn)
			return (0);
		else if (value < crit)
			return (1);
	} else {
		if (value > warn)
			return (0);
		else if (value > crit)
			return (1);
	}
	return (2);
}

/*
 * Print the appriate message according to the status level.  Exit with
 * the correct return code.
 */
static void
message_and_exit(int level, intmax_t value, const char *info)
{

	if (level == 0)
		printf("OK: ");
	else if (level == 1)
		printf("Warning: ");
	else if (level == 2)
		printf("Critical: ");
	else
		printf("Uknown: ");

	printf("%ju %s\n", value, info);
	exit(level);
}

/*
 * Check the statistics for the requested parameter.
 */
static void
check_stats(struct varnish_stats *VSL_stats, char *param, int w, int c, int less)
{
	int level;

	if (!strcmp(param, "ratio")) {
		int64_t total = VSL_stats->cache_hit + VSL_stats->cache_miss;
		double ratio = 0;

		if (total > 0)
			ratio = 100.0 * VSL_stats->cache_hit / total;
		level = check_treshold(ratio, w, c, less);
		message_and_exit(level, ratio, "Cache hit ratio");
	}
#define MAC_STAT(n, t, f, d) \
	do { \
		intmax_t ju = VSL_stats->n; \
		if (!strcmp(param, #n)) { \
			level = check_treshold(ju, w, c, less); \
			message_and_exit(level, ju, d); \
		} \
	} while (0);
#include "stat_field.h"
#undef MAC_STAT
	printf("Invalid parameter: %s\n", param);
	exit(3);
}

/*-------------------------------------------------------------------------------*/

static void
help(void)
{
	fprintf(stderr, "usage: "
	    "check_varnish [-l] [-n varnish_name] [-p param_name [-c N] [-w N]]\n"
	    "\n"
	    "-l              Warn when the measured value is less, not more,\n"
	    "                than the configured threshold.\n"
	    "-n varnish_name Specify the Varnish instance name\n"
	    "-p param_name   Specify the parameter to check (see below).\n"
	    "                Default is 'ratio'.\n"
	    "-c N            Set critical treshold to N\n"
	    "-w N            Set warning threshold to N\n"
	    "\n"
	    "All items reported by varnishstat(1) are available - use the\n"
	    "identifier listed in the left column by 'varnishstat -l'.  In\n"
	    "addition, the following parameters are available:\n"
	    "\n"
	    "ratio   The cache hit ratio expressed as a percentage of hits to\n"
	    "        hits + misses.  Default thresholds are 95 and 90.\n"
	);
	exit(0);
}

static void
usage(void)
{

	fprintf(stderr, "usage: "
	    "check_varnish [-l] [-n varnish_name] [-p param_name [-c N] [-w N]]\n");
	exit(3);
}


int
main(int argc, char **argv)
{
	struct varnish_stats *VSL_stats;
	int critical = 0, warning = 0;
	const char *n_arg = NULL;
	char *param = NULL;
	int less = 0;
	int opt;

	while ((opt = getopt(argc, argv, "c:hln:p:w:")) != -1) {
		switch (opt) {
		case 'c':
			critical = atoi(optarg);
			break;
		case 'h':
			help();
			break;
		case 'l':
			less = 1;
			break;
		case 'n':
			n_arg = optarg;
			break;
		case 'p':
			param = strdup(optarg);
			break;
		case 'w':
			warning = atoi(optarg);
			break;
		default:
			usage();
		}
	}

	if ((VSL_stats = VSL_OpenStats(n_arg)) == NULL)
		exit(1);

	/* Default: if no param specified, check hit ratio.  If no warning
	 * and critical values are specified either, set these to default.
	 */
	if (param == NULL) {
		param = strdup("ratio");
		if (!warning && !critical) {
			warning = 95;
			critical = 90;
			less = 1;
		}
	}

	if (!param || (!critical && !warning))
		usage();

	check_stats(VSL_stats, param, warning, critical, less);

	exit(0);
}
