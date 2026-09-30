#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>

/**
 * cd /tmp
 * mkdir empty_dir
 * rsync -a --delete empty_dir/    test/
*/
char TEST_DIR[100] = "/tmp/test/";
void *thread_function(void *arg)
{
	long tid = (long)arg;

	printf("Thread %ld starting\n", tid);

	for (size_t i = 0; i < 100000000; i++) {
		if (i % 10000 == 0)
			printf("[martins3:%s:%d] %ld %ld\n", __FUNCTION__,
			       __LINE__, tid, i);
		FILE *file;
		// Open file for writing
		char name[200];
		int ret =
			snprintf(name, 200, "%s/%ld-%ld.txt", TEST_DIR, i, tid);
		if (ret < 0) {
			perror("Error opening file");
			pthread_exit(NULL);
		}
		file = fopen(name, "w");
		if (file == NULL) {
			perror("Error opening file");
			pthread_exit(NULL);
		}
		// Write data to file
		// fprintf(file, "Hello World!");
		// Close file
		fclose(file);
	}

	printf("Thread %ld exiting\n", tid);

	pthread_exit(NULL);
}

int main()
{
	// Create the directory
	if (mkdir(TEST_DIR, 0775) == -1 && errno != EEXIST) {
		perror("Error creating directory");
		return 1;
	}

	pthread_t threads[2];
	long tids[2];

	// create first thread
	tids[0] = 0;
	pthread_create(&threads[0], NULL, thread_function, (void *)tids[0]);

	// create second thread
	tids[1] = 1;
	pthread_create(&threads[1], NULL, thread_function, (void *)tids[1]);

	// wait for threads to finish
	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);

	return 0;
}
