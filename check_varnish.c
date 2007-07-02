/*-
 * Copyright (c) 2006-2007 Linpro AS
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



static int
check_treshold(int value, int warn, int crit, int less)
{
	if (!less) {
		if (value < warn)
			return 0;
		else if (value < crit)
			return 1;
	}
	else {
		if (value > warn)
			return 0;
		else if (value > crit)
			return 1;
	}
	return 2;
	
}

static void
message_and_exit(int level, int value, char *info)
{
	if (level == 0)
		printf("OK: ");
	else if (level == 1)
		printf("Warning: ");
	else if (level == 2)
		printf("Critical: ");
	else
		printf("Uknown: ");
		
	printf("%d %s\n", value, info);
	exit(level);
}

static void
check_stats(struct varnish_stats *VSL_stats, char *param, int w, int c, int less)
{
#define MAC_STAT(n, t, f, d) \
	do { \
		intmax_t ju = VSL_stats->n; \
		int level; \
		if (!strcmp(param, #n)) { \
			level = check_treshold(ju, w, c, less); \
			message_and_exit(level, ju, d); \
		} \
	} while (0);
#include "stat_field.h"
#undef MAC_STAT
}

static void
help(void)
{
	fprintf(stderr, "usage: check_varnish -p param_name -c N -w N [-l] [-n varnish_name] [-v]\n"
	 "Valid options:\n"
	"-c N\t\t warn as critical at treshold N\n"
	"-l\t\t specify that values should be less than tresholds for warnings to be issued\n"
	"-n varnish_name\t specify varnish instance name\n"
	"-p param_name\t specify the parameter to check. See valid parameters\n"
	"-v\t\t print verbose output. Can be specified up to three times\n"
	"-w N\t\t warn as warning at treshold N\n"
	);
	exit(0);
}

static void
usage(void)
{
	fprintf(stderr, "usage: check_varnish -p param_name -c N -w N [-l] [-n varnish_name] [-v]\n");
	exit(3);
}


int
main(int argc, char **argv)
{
	int c;
	struct varnish_stats *VSL_stats;
	int critical = 0, warning = 0;
	int verbose = 0;
	const char *n_arg = NULL;
	char *param = NULL;
	int less = 0;

	while ((c = getopt(argc, argv, "c:hln:p:vw:")) != -1) {
		switch (c) {
		case 'c':
			critical = atoi(optarg);
			break;
		case 'h':
			help();
		case 'l':
			less = 1;
			break;
		case 'n':
			n_arg = optarg;
			break;
		case 'p':
			param = strdup(optarg);
			break;
		case 'v':
			verbose++;
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

	check_stats(VSL_stats, param, warning, critical, less);

	exit(0);
}
