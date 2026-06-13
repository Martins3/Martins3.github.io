# Threads: Thread Synchronization

## 30.1 Protecting Accesses to Shared Variables: Mutexes

### 30.1.1 Statically Allocated Mutexes
```plain
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
```

### 30.1.2 Locking and Unlocking a Mutex
```plain
#include <pthread.h>
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
//Both return 0 on success, or a positive error number on error
# Linux Programming Interface: Chapter 30

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
