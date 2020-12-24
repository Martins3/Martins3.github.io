# 30 Thread:Thread Synchronization

## 30.1 Protecting Accesses to Shared Variables: Mutexes

### 30.1.1 Statically Allocated Mutexes
```
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
```

### 30.1.2 Locking and Unlocking a Mutex
```
#include <pthread.h>
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
//Both return 0 on success, or a positive error number on error
# Linux Programming Interface: Chapter 30
