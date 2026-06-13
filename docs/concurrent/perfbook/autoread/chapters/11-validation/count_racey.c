/*
 * count_racey.c: Demonstrate race condition in non-atomic counter.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Copyright (c) 2025
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>

static unsigned long counter = 0;
static unsigned long delay_us = 0;
static int nr_threads = 4;
static unsigned long nr_inc_per_thread = 1000000;

static void *inc_count(void *arg)
{
	unsigned long i;
	(void)arg;

	for (i = 0; i < nr_inc_per_thread; i++) {
		unsigned long tmp = counter;
		if (delay_us)
			usleep(delay_us);
		counter = tmp + 1;
	}
	return NULL;
}

static unsigned long read_count(void)
{
	return counter;
}

int main(int argc, char *argv[])
{
	pthread_t *threads;
	unsigned long expected;
	unsigned long actual;
	int i;

	if (argc > 1)
		nr_threads = atoi(argv[1]);
	if (argc > 2)
		nr_inc_per_thread = strtoul(argv[2], NULL, 0);
	if (argc > 3)
		delay_us = strtoul(argv[3], NULL, 0);

	printf("threads=%d increments=%lu delay_us=%lu\n",
	       nr_threads, nr_inc_per_thread, delay_us);

	threads = malloc(sizeof(*threads) * nr_threads);
	if (!threads) {
		perror("malloc");
		return 1;
	}

	for (i = 0; i < nr_threads; i++) {
		if (pthread_create(&threads[i], NULL, inc_count, NULL) != 0) {
			perror("pthread_create");
			return 1;
		}
	}

	for (i = 0; i < nr_threads; i++)
		pthread_join(threads[i], NULL);

	expected = nr_threads * nr_inc_per_thread;
	actual = read_count();

	printf("expected=%lu actual=%lu lost=%lu\n",
	       expected, actual, expected - actual);

	free(threads);
	return 0;
}
