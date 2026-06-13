/* Copied from shmop(2) */

/* svshm_string_read.c

   Licensed under GNU General Public License v2 or later.
*/
#include "svshm_string.h"

int main(int argc, char *argv[]) {
  int semid, shmid;
  union semun arg, dummy;
  struct sembuf sop;
  char *addr;

  /* Create shared memory and semaphore set containing one
     semaphore. */

  shmid = shmget(IPC_PRIVATE, MEM_SIZE, IPC_CREAT | 0600);
  if (shmid == -1)
    errExit("shmget");

  semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
  if (semid == -1)
    errExit("semget");

  /* Attach shared memory into our address space. */

  addr = shmat(shmid, NULL, SHM_RDONLY);
  if (addr == (void *)-1)
    errExit("shmat");

  /* Initialize semaphore 0 in set with value 1. */

  arg.val = 1;
  if (semctl(semid, 0, SETVAL, arg) == -1)
    errExit("semctl");

  printf("shmid = %d; semid = %d\n", shmid, semid);

  /* Wait for semaphore value to become 0. */

  sop.sem_num = 0;
  sop.sem_op = 0;
  sop.sem_flg = 0;

  if (semop(semid, &sop, 1) == -1)
    errExit("semop");

  /* Print the string from shared memory. */

  printf("%s\n", addr);

  /* Remove shared memory and semaphore set. */

  if (shmctl(shmid, IPC_RMID, NULL) == -1)
    errExit("shmctl");
  if (semctl(semid, 0, IPC_RMID, dummy) == -1)
    errExit("semctl");

  exit(EXIT_SUCCESS);
}
