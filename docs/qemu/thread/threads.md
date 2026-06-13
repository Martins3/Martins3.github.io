## qemu 到底有那些 thread
<!-- 841889be-f42d-453d-8160-a499075e2305 -->

```txt
gdb) info threads
  Id   Target Id                                            Frame
* 1    Thread 0xffff7f98e500 (LWP 771090) "qemu-system-aar" 0x0000ffff7dc926fc in ppoll () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  2    Thread 0xfffd557cea60 (LWP 771112) "worker"          0x0000ffff7dc2b4e0 in __futex_abstimed_wait_cancelable64 () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  3    Thread 0xfffd55fdea60 (LWP 771111) "worker"          0x0000ffff7dc2b4e0 in __futex_abstimed_wait_cancelable64 () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  4    Thread 0xfffd567eea60 (LWP 771109) "worker"          0x0000ffff7dc2b4e0 in __futex_abstimed_wait_cancelable64 () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  5    Thread 0xffff7abeca60 (LWP 771108) "worker"          0x0000ffff7dc2b4e0 in __futex_abstimed_wait_cancelable64 () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  6    Thread 0xfffd56ffea60 (LWP 771099) "vnc_worker"      0x0000ffff7dc2b4e0 in __futex_abstimed_wait_cancelable64 () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  7    Thread 0xfffd66fd9a60 (LWP 771098) "CPU 1/KVM"       0x0000ffff7dc98fd0 in ioctl () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  8    Thread 0xfffd677e9a60 (LWP 771097) "CPU 0/KVM"       0x0000ffff7dc98fd0 in ioctl () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  9    Thread 0xfffd67ff9a60 (LWP 771096) "IO mon_iothread" 0x0000ffff7dc926fc in ppoll () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  10   Thread 0xffff7f98e500 (LWP 771095) "vhost-771090"    0x0000000000000000 in ?? ()
  11   Thread 0xffff7f98e500 (LWP 771094) "vhost-771090"    0x0000000000000000 in ?? ()
  12   Thread 0xffff7b4fda60 (LWP 771092) "qemu-system-aar" 0x0000ffff7dc926fc in ppoll () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
  13   Thread 0xffff7be0ea60 (LWP 771091) "qemu-system-aar" 0x0000ffff7dc9bee4 in syscall () from /nix/store/r7pnxs3cfl3qxwacj38iakpm5v1ch6lz-glibc-2.40-66/lib/libc.so.6
```
- 1 是 main loop
- 12 virtio_blk_io0
- 13 rcu

在 `qemu_thread_create` 中添加调试语句:
```txt
call_rcu
IO virtio_blk_io0
worker
IO mon_iothread
CPU 0/KVM
CPU 1/KVM
```

作为热迁移 target 端启动，但是还没有发起热迁移的时候:
```txt
info threads
  Id   Target Id                                            Frame
* 1    Thread 0x7ffff4efedc0 (LWP 236075) "qemu-system-x86" 0x00007ffff650da0e in ppoll ()
  2    Thread 0x7ffff4efd6c0 (LWP 236078) "qemu-system-x86" 0x00007ffff651961d in syscall ()
  3    Thread 0x7fffeffff6c0 (LWP 236079) "qemu-system-x86" 0x00007ffff650da0e in ppoll ()
  4    Thread 0x7fffef7fe6c0 (LWP 236081) "worker"          0x00007ffff649450e in __futex_abstimed_wait_common ()
  5    Thread 0x7fffecffb6c0 (LWP 236086) "IO mon_iothread" 0x00007ffff650da0e in ppoll ()
  6    Thread 0x7fffe7fff6c0 (LWP 236087) "CPU 0/KVM"       0x00007ffff649450e in __futex_abstimed_wait_common ()
  7    Thread 0x7fffe77fe6c0 (LWP 236088) "CPU 1/KVM"       0x00007ffff649450e in __futex_abstimed_wait_common ()
  8    Thread 0x7ffddebff6c0 (LWP 236091) "vnc_worker"      0x00007ffff649450e in __futex_abstimed_wait_common ()
```

当发起热迁移的时候:

在 target 端:
```txt
  Id   Target Id                                            Frame
* 1    Thread 0x7ffff4efedc0 (LWP 240613) "qemu-system-x86" 0x00007ffff650da0e in ppoll ()
  2    Thread 0x7ffff4efd6c0 (LWP 240616) "qemu-system-x86" 0x00007ffff651961d in syscall ()
  3    Thread 0x7fffeffff6c0 (LWP 240617) "qemu-system-x86" 0x00007ffff650da0e in ppoll ()
  5    Thread 0x7fffecffb6c0 (LWP 240623) "IO mon_iothread" 0x00007ffff650da0e in ppoll ()
  6    Thread 0x7fffe7fff6c0 (LWP 240624) "CPU 0/KVM"       0x00007ffff649450e in __futex_abstimed_wait_common ()
  7    Thread 0x7fffe77fe6c0 (LWP 240625) "CPU 1/KVM"       0x00007ffff649450e in __futex_abstimed_wait_common ()
  8    Thread 0x7ffddebff6c0 (LWP 240628) "vnc_worker"      0x00007ffff649450e in __futex_abstimed_wait_common ()
  9    Thread 0x7ffdde3fe6c0 (LWP 241156) "mig/dst/recv_0"  0x00007ffff651e117 in recvmsg ()
  10   Thread 0x7ffdddbfd6c0 (LWP 241157) "mig/dst/recv_1"  0x00007ffff651e117 in recvmsg ()
  11   Thread 0x7ffddd3fc6c0 (LWP 241158) "mig/dst/recv_2"  0x00007ffff651e117 in recvmsg ()
  12   Thread 0x7ffddcbfb6c0 (LWP 241160) "mig/dst/recv_3"  0x00007ffff651e117 in recvmsg ()
```

在 source 端:
```txt
 Id   Target Id                                            Frame
* 1    Thread 0x7ffff4efedc0 (LWP 238966) "qemu-system-x86" 0x00007ffff650da0e in ppoll ()
  2    Thread 0x7ffff4efd6c0 (LWP 238969) "qemu-system-x86" 0x00007ffff651961d in syscall ()
  3    Thread 0x7fffeffff6c0 (LWP 238970) "qemu-system-x86" 0x00007ffff650da0e in ppoll ()
  5    Thread 0x7fffecffb6c0 (LWP 238979) "IO mon_iothread" 0x00007ffff650da0e in ppoll ()
  6    Thread 0x7fffe7fff6c0 (LWP 238980) "CPU 0/KVM"       0x00007ffff6516f0f in ioctl ()
  7    Thread 0x7fffe77fe6c0 (LWP 238981) "CPU 1/KVM"       0x00007ffff6516f0f in ioctl ()
  8    Thread 0x7ffddebff6c0 (LWP 238984) "vnc_worker"      0x00007ffff649450e in __futex_abstimed_wait_common ()
  17   Thread 0x7ffddd3fc6c0 (LWP 241142) "mig/src/main"    0x00007ffff649450e in __futex_abstimed_wait_common ()
  20   Thread 0x7ffdddbfd6c0 (LWP 241147) "mig/src/send_0"  0x00007ffff649450e in __futex_abstimed_wait_common ()
  22   Thread 0x7ffdde3fe6c0 (LWP 241151) "mig/src/send_1"  0x00007ffff649450e in __futex_abstimed_wait_common ()
  24   Thread 0x7fffee7fc6c0 (LWP 241154) "mig/src/send_2"  0x00007ffff649450e in __futex_abstimed_wait_common ()
  25   Thread 0x7fffedffb6c0 (LWP 241155) "mig/src/send_3"  0x00007ffff649450e in __futex_abstimed_wait_common ()
```

热迁移的参数为:
```c
			local cmds=(
				"migrate_set_capability multifd on"
				"migrate_set_parameter max-bandwidth 1 "
				"migrate_set_parameter multifd-channels 4"
				"migrate_set_parameter multifd-compression zstd"
				"migrate_incoming unix:$vm_dir/migrate.sock"
			)
```

所以，正好会构建出来 4 个额外的 thread 来发送和接受，这是预期的。

发送端的结果为:
- __clone3
  - start_thread
    - qemu_thread_start
      - multifd_send_thread
        - qemu_sem_wait
          - qemu_cond_wait_impl
            - pthread_cond_wait@@GLIBC_2.3.2
              - __futex_abstimed_wait_common
                - __futex_abstimed_wait_common

接收端的结果为:
- __clone3
  - start_thread
    - qemu_thread_start
      - multifd_recv_thread
        - qio_channel_readv_full_all_eof
          - qio_channel_socket_readv
            - recvmsg

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
