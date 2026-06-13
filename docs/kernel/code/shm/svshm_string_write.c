/* Copied from shmop(2) */
/* svshm_string_write.c

   Licensed under GNU General Public License v2 or later.
*/
#include "svshm_string.h"

int main(int argc, char *argv[]) {
  int semid, shmid;
  struct sembuf sop;
  char *addr;
  size_t len;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s shmid semid string\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  len = strlen(argv[3]) + 1; /* +1 to include trailing '\0' */
  if (len > MEM_SIZE) {
    fprintf(stderr, "String is too big!\n");
    exit(EXIT_FAILURE);
  }

  /* Get object IDs from command-line. */

  shmid = atoi(argv[1]);
  semid = atoi(argv[2]);

  /* Attach shared memory into our address space and copy string
     (including trailing null byte) into memory. */

  addr = shmat(shmid, NULL, 0);
  if (addr == (void *)-1)
    errExit("shmat");

  memcpy(addr, argv[3], len);

  /* Decrement semaphore to 0. */

  sop.sem_num = 0;
  sop.sem_op = -1;
  sop.sem_flg = 0;

  if (semop(semid, &sop, 1) == -1)
    errExit("semop");

  exit(EXIT_SUCCESS);
}
