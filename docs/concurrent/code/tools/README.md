## tsan
<!-- baf35d94-b845-4ed5-962b-1ba006ee2e5a -->

```txt
🧀  make tsan
clang -fsanitize=thread -O3 -g drd.c -lpthread -o drd.out
./drd.out
保护模式已关闭（数据争用将会发生）。
==================
WARNING: ThreadSanitizer: data race (pid=203647)
  Write of size 4 at 0x55edcc3b0768 by thread T2:
    #0 worker /home/martins3/data/vn/code/src/concurrent/valgrind/drd.c:23:23 (drd.out+0x106a0c)

  Previous write of size 4 at 0x55edcc3b0768 by thread T1:
    #0 worker /home/martins3/data/vn/code/src/concurrent/valgrind/drd.c:23:23 (drd.out+0x106a0c)

  Location is global 'shared_counter' of size 4 at 0x55edcc3b0768 (drd.out+0x14b4768)

  Thread T2 (tid=203650, running) created by main thread at:
    #0 pthread_create <null> (drd.out+0x7afc5)
    #1 main /home/martins3/data/vn/code/src/concurrent/valgrind/drd.c:56:9 (drd.out+0x106b00)

  Thread T1 (tid=203649, finished) created by main thread at:
    #0 pthread_create <null> (drd.out+0x7afc5)
    #1 main /home/martins3/data/vn/code/src/concurrent/valgrind/drd.c:52:9 (drd.out+0x106ae0)

SUMMARY: ThreadSanitizer: data race /home/martins3/data/vn/code/src/concurrent/valgrind/drd.c:23:23 in worker
==================

最终计数值: 2000000
理论期望值: 2000000
```

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
