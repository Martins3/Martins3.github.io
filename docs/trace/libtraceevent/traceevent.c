// https://man7.org/linux/man-pages/man3/libtraceevent.3.html
//
// COPY FROM https://github.com/rostedt/libtraceevent/blob/libtraceevent/samples/test-event.c

// SPDX-License-Identifier: LGPL-2.1
/*
 * Copyright (C) 2022 Google Inc, Steven Rostedt <rostedt@goodmis.org>
 */
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <traceevent/event-parse.h>

static char *argv0;

static char *get_this_name(void)
{
	static char *this_name;
	char *arg;
	char *p;

	if (this_name)
		return this_name;

	arg = argv0;
	p = arg + strlen(arg);

	while (p >= arg && *p != '/')
		p--;
	p++;

	this_name = p;
	return p;
}

static void usage(void)
{
	char *p = get_this_name();

	printf("usage: %s [options]\n"
	       " -h : this message\n"
	       " -s system : the system for the event\n"
	       " -e format : the event format file\n"
	       " -f file : file to read the event from\n"
	       "      otherwise, reads from stdin\n"
	       "\n",
	       p);
	exit(-1);
}

static void __vdie(const char *fmt, va_list ap, int err)
{
	int ret = errno;
	char *p = get_this_name();

	if (err && errno)
		perror(p);
	else
		ret = -1;

	fprintf(stderr, "  ");
	vfprintf(stderr, fmt, ap);

	fprintf(stderr, "\n");
	exit(ret);
}

void die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	__vdie(fmt, ap, 0);
	va_end(ap);
}

void pdie(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	__vdie(fmt, ap, 1);
	va_end(ap);
}

/* Must be a power of two */
#define BUFALLOC 1024
#define BUFMASK (~(BUFALLOC - 1))

int main(int argc, char **argv)
{
	struct tep_handle *tep;
	struct tep_event *event;
	FILE *file = stdin;
	FILE *fp = NULL;
	char *system = NULL;
	char *event_buf = NULL;
	int esize = 0;
	int c;

	argv0 = argv[0];

	while ((c = getopt(argc, argv, "hs:e:f:")) >= 0) {
		switch (c) {
		case 's':
			system = optarg;
			break;
		case 'e':
			event_buf = optarg;
			file = NULL;
			break;
		case 'f':
			fp = fopen(optarg, "r");
			if (!fp)
				pdie("%s", optarg);
			file = fp;
			break;
		case 'h':
			usage();
		}
	}
	if (file) {
		char *line = NULL;
		size_t n = 0;
		int len;

		while (getline(&line, &n, file) > 0) {
			len = strlen(line) + 1;

			if (((esize - 1) & BUFMASK) <
			    ((esize + len) & BUFMASK)) {
				int a;

				a = (esize + len + BUFALLOC - 1) & BUFMASK;
				event_buf = realloc(event_buf, a);
				if (!event_buf)
					pdie("allocating event");
			}
			strcpy(event_buf + esize, line);
			esize += len - 1;
		}
		free(line);
	}

	tep = tep_alloc();
	if (!tep)
		pdie("Allocating tep handle");

	tep_set_loglevel(TEP_LOG_ALL);

	if (!system)
		system = "test";

	if (tep_parse_format(tep, &event, event_buf, esize, system))
		die("Failed to parse event");
}
