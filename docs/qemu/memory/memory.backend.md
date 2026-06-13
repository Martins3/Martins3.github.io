# qemu memory backend


源码中 ./backends 

-  hostmem-epc.c
-  hostmem-file.c
-  hostmem-memfd.c
-  hostmem-ram.c
-  hostmem-shm.c
-  hostmem.c
-  iommufd.c


https://www.qemu.org/docs/master/system/devices/vhost-user.html 中提到，只有这几种 backend 才可以

> In order for the daemon to access the VirtIO queues to process the requests it needs access to the guest’s address space. This is achieved via the memory-backend-file, memory-backend-memfd, or memory-backend-shm objects.

## 基本操作
1. info memdev

```txt
(qemu) info memdev
memory backend: mem0
  size:  8589934592
  merge: true
  dump: true
  prealloc: false
  share: true
  reserve: true
  policy: default
  host nodes:
```


执行流程:

启动的时候:
- main 
  - qemu_init 
    - qemu_create_late_backends 
      - object_option_foreach_add 
        - user_creatable_add_qapi 
          - user_creatable_add_type 
            - user_creatable_complete 
              - host_memory_backend_memory_complete 
                - memfd_backend_memory_alloc 

只会调用一次，而且热插不会调用，看来是通过 maxmem 一次性就分配好了。

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
