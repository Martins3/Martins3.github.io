/*
 * correlated_fields.c - RCU + indirection for correlated field updates.
 * Corresponds to perfbook Section 13.5.4 "Correlated Fields".
 *
 * Problem: an animal structure has three correlated measurements.
 * Updating them one-by-one could let readers see inconsistent values.
 * Solution: place correlated fields in a separate structure, update
 * the pointer with rcu_assign_pointer().
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "urcu_simple.h"

static pthread_mutex_t rcu_global_lock = PTHREAD_MUTEX_INITIALIZER;

struct measurement {
	double meas_1;
	double meas_2;
	double meas_3;
};

struct animal {
	char name[40];
	struct measurement *mp;  /* RCU-protected pointer */
};

static struct animal my_animal = {
	.name = "Schroedinger's Cat",
	.mp = NULL,
};

static void init_animal(void)
{
	struct measurement *m = malloc(sizeof(*m));
	m->meas_1 = 1.0;
	m->meas_2 = 2.0;
	m->meas_3 = 3.0;
	my_animal.mp = m;
}

/* Update measurements atomically w.r.t readers */
static void update_measurements(double v1, double v2, double v3)
{
	struct measurement *new_m = malloc(sizeof(*new_m));
	new_m->meas_1 = v1;
	new_m->meas_2 = v2;
	new_m->meas_3 = v3;

	pthread_mutex_lock(&rcu_global_lock);
	struct measurement *old_m = my_animal.mp;
	rcu_assign_pointer(my_animal.mp, new_m);
	pthread_mutex_unlock(&rcu_global_lock);

	synchronize_rcu();
	free(old_m);
}

static void *reader_thread(void *arg)
{
	(void)arg;
	for (int i = 0; i < 100000; i++) {
		rcu_read_lock();
		struct measurement *m = rcu_dereference(my_animal.mp);
		/* Readers see a consistent triple */
		double a = m->meas_1;
		double b = m->meas_2;
		double c = m->meas_3;
		rcu_read_unlock();

		/* Validate consistency: they should be scaled together */
		if (a * 2.0 != b || a * 3.0 != c) {
			printf("[FAIL] Inconsistent read: %.2f %.2f %.2f\n", a, b, c);
			return NULL;
		}
	}
	return (void *)1;
}

static void *writer_thread(void *arg)
{
	(void)arg;
	for (int i = 0; i < 500; i++) {
		update_measurements(10.0, 20.0, 30.0);
		update_measurements(1.0, 2.0, 3.0);
	}
	return (void *)1;
}

int main(void)
{
	printf("[correlated_fields] RCU + indirection for correlated updates\n");
	init_animal();

	pthread_t r1, r2, w;
	pthread_create(&r1, NULL, reader_thread, NULL);
	pthread_create(&r2, NULL, reader_thread, NULL);
	pthread_create(&w, NULL, writer_thread, NULL);

	void *rv1, *rv2, *wv;
	pthread_join(r1, &rv1);
	pthread_join(r2, &rv2);
	pthread_join(w, &wv);

	if (rv1 && rv2 && wv)
		printf("[OK] All readers saw consistent measurement sets\n");
	else
		printf("[FAIL] Some thread reported error\n");

	free(my_animal.mp);
	return (rv1 && rv2 && wv) ? 0 : 1;
}
