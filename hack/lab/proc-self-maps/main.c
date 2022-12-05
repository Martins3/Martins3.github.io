#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void print_maps() {
  FILE *map;
  map = fopen("/proc/self/maps", "r");

  char line[512];

  pthread_mutex_lock(&mutex);
  while (!feof(map)) {
    if (fgets(line, 512, map) == NULL)
      break;
    printf("%s", line);
  }
  pthread_mutex_unlock(&mutex);
}

void * print_message_function(void *ptr) {
  char *message;
  message = (char *)ptr;

	void *addr = mmap(NULL, 1 << 20,
		   PROT_READ | PROT_WRITE,
		    MAP_PRIVATE |
		   MAP_ANONYMOUS, -1, 0);

  printf("%s %p\n", message, addr);
  sleep(10);
  // print_maps();
  return NULL;
}


int main() {
  pthread_t thread1, thread2;
  char *message1 = "Thread 1";
  char *message2 = "Thread 2";
  int iret1, iret2;

  /* Create independent threads each of which will execute function */

  iret1 = pthread_create(&thread1, NULL, 
      print_message_function, (void *)message1);
  iret2 = pthread_create(&thread2, NULL,
      print_message_function, (void *)message2);

  /* Wait till threads are complete before main continues. Unless we  */
  /* wait we run the risk of executing an exit which will terminate   */
  /* the process and all threads before the threads have completed.   */

  print_maps();

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  printf("Thread 1 returns: %d\n", iret1);
  printf("Thread 2 returns: %d\n", iret2);
  return 0;
}

