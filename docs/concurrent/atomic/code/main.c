#define _GNU_SOURCE
#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h> //cpu_set_t , CPU_SET
#include <pthread.h> //pthread_t
#include <stdio.h>
#include <stdint.h>

#include <unistd.h>

/*
 * @loop : how many times to loop
 * @addr : addr to write
 * @data : data to write
 */
long test_store_buffer(long loop, long *addr, long data);

struct thread_data {
	pthread_t thread;
	long *addr;
	long data;
	int core_id;
};

struct thread_data *tda;
int t_nr;
#define for_each_thread(i) for (size_t i = 0; i < t_nr; i++)

static int stick_this_thread_to_core(int core_id)
{
	if (core_id == -1)
		return 0;
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(core_id, &cpuset);

	pthread_t current_thread = pthread_self();
	return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t),
				      &cpuset);
}

static void *store_buffer_func(void *arg)
{
	struct timeval tv1;
	struct timeval tv2;
	struct thread_data *td = &(tda[(long int)arg]);
	if (stick_this_thread_to_core(td->core_id))
		return NULL;

	gettimeofday(&tv1, NULL);
	long diff = test_store_buffer(0x800000000, td->addr, td->data);
	gettimeofday(&tv2, NULL);

	printf("Total time = %f seconds\n",
	       (double)(tv2.tv_usec - tv1.tv_usec) / 1000000 +
		       (double)(tv2.tv_sec - tv1.tv_sec));
	printf("Diff = %ld\n", diff);
	return NULL;
}

int init_same_address()
{
	void *data;
	if (posix_memalign(&data, getpagesize(), getpagesize()))
		return EXIT_FAILURE;

	for_each_thread(i) {
		tda[i].data = 0x1234 + i;
		tda[i].addr = data;
	}
	return EXIT_SUCCESS;
}

int init_diff_address()
{
	for_each_thread(i) {
		void *data;
		if (posix_memalign(&data, getpagesize(), getpagesize()))
			return EXIT_FAILURE;
		tda[i].data = 0x1234 + i;
		tda[i].addr = data;
	}

	return EXIT_SUCCESS;
}

int test_store_buffer_and_cache_cohenrency()
{
	int s;
	init_same_address();
	/* init_diff_address(); */
	for_each_thread(i) {
		tda[i].core_id = -1;
		if (i == 0)
			tda[i].core_id = 0;
		if (i == 1)
			tda[i].core_id = 3;
		s = pthread_create(&tda[i].thread, NULL, store_buffer_func,
				   (void *)i);
		if (s)
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

void martins3_atomic_add(volatile uint64_t *data, long add);
void martins3_atomic_add2(volatile uint64_t *data, long add);
void martins3_add(volatile uint64_t *data, long add);

static volatile uint64_t v = 0;
void *atomic_func(void *x)
{
	struct timeval tv1;
	struct timeval tv2;
	long idx = (long)x;
	struct thread_data *td = &(tda[idx]);
	if (stick_this_thread_to_core(td->core_id))
		return NULL;

	gettimeofday(&tv1, NULL);

#define WHICH_TO_TEST 2

	for (size_t i = 0; i < 0x10000000; i++)
#if WHICH_TO_TEST == 1
		// 不敢相信，居然 __sync_add_and_fetch 提供了 atomic inc 的功能
		__sync_add_and_fetch(&v, 1);
#elif WHICH_TO_TEST == 2
		/* martins3_atomic_add2(&v, 1); */
		__atomic_add_fetch(&v, 1, __ATOMIC_RELAXED);
#elif WHICH_TO_TEST == 3
		martins3_add(&v, 1);
#endif
	gettimeofday(&tv2, NULL);

	printf("Total time = %f seconds\n",
	       (double)(tv2.tv_usec - tv1.tv_usec) / 1000000 +
		       (double)(tv2.tv_sec - tv1.tv_sec));
	return 0;
}

int test_atomic()
{
	int s;
	for_each_thread(i) {
		if (i == 0)
			tda[i].core_id = 0;
		if (i == 1)
			tda[i].core_id = 3;
		s = pthread_create(&tda[i].thread, NULL, atomic_func,
				   (void *)i);
		if (s)
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	long number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
	t_nr = number_of_processors;
	t_nr = 2;
	tda = malloc(sizeof(struct thread_data) * t_nr);
	if (tda == NULL)
		return EXIT_FAILURE;

#define TEST_WHAT 2
#if TEST_WHAT == 1
	if (test_store_buffer_and_cache_cohenrency())
		return EXIT_FAILURE;
#endif

#if TEST_WHAT == 2
	if (test_atomic())
		return EXIT_FAILURE;
#endif

	for_each_thread(i) {
		if (pthread_join(tda[i].thread, NULL))
			return EXIT_FAILURE;
	}
#if TEST_WHAT == 2
	printf("v : %lx\n", v);
#endif
	return EXIT_SUCCESS;
}
