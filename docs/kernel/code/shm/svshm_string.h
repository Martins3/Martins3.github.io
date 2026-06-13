/* Copied from shmop(2) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>

#define errExit(msg)                                                           \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

union semun { /* Used in calls to semctl() */
  int val;
  struct semid_ds *buf;
  unsigned short *array;
#if defined(__linux__)
  struct seminfo *__buf;
#endif
};

#define MEM_SIZE 4096
