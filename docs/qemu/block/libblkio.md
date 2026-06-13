## libblkio
https://kvm-forum.qemu.org/2022/libblkio-kvm-forum-2022.pdf

virtio-blk + VFIO PCI to bypass the guest kernel’s virtio-blk drive

https://gitlab.com/libblkio/libblkio.git/

libblkio qemu iouring guest 中 4k randread 的结果:
```txt
   - qemu_main_loop                                                                                                                                                  ▒
      - 54.46% main_loop_wait                                                                                                                                        ▒
         - 53.89% os_host_main_loop_wait (inlined)                                                                                                                   ▒
            - 46.13% glib_pollfds_poll (inlined)                                                                                                                     ▒
               - 44.81% g_main_context_dispatch                                                                                                                      ▒
                  - 44.81% g_main_context_dispatch_unlocked                                                                                                          ▒
                     - 44.76% aio_ctx_dispatch                                                                                                                       ▒
                        - 44.71% aio_dispatch                                                                                                                        ▒
                           - 44.69% aio_dispatch_handlers (inlined)                                                                                                  ▒
                              - aio_dispatch_handler                                                                                                                 ▒
                                 - 41.86% virtio_queue_notify_vq (inlined)                                                                                           ▒
                                    - 41.85% virtio_blk_handle_vq                                                                                                    ▒
                                       - 36.29% defer_call_end                                                                                                       ▒
                                          - 36.28% blkio_deferred_fn                                                                                                 ▒
                                             - 36.27% blkioq_do_io                                                                                                   ▒
                                                  blkio::handling_result_with_value (inlined)                                                                        ▒
                                                - blkio::blkioq_do_io::_$u7b$$u7b$closure$u7d$$u7d$::ha60b5b610ff6645a (inlined)                                     ▒
                                                   - 36.26% blkio::Blkioq::do_io                                                                                     ▒
                                                      - <blkio::drivers::iouring::IoUringQueue as blkio::Queue>::do_io                                               ▒
                                                         - 36.24% io_uring::submit::Submitter::enter (inlined)                                                       ▒
                                                            - io_uring::sys::io_uring_enter (inlined)                                                                ▒
                                                               - 36.24% syscall                                                                                      ▒
                                                                  - 36.22% entry_SYSCALL_64_after_hwframe                                                            ▒
                                                                     - do_syscall_64                                                                                 ▒
                                                                        - 36.19% __do_sys_io_uring_enter                                                             ▒
                                                                           - 36.11% io_submit_sqes                                                                   ▒
                                                                              - 34.30% io_issue_sqe                                                                  ▒
                                                                                 - 33.43% io_read                                                                    ▒
                                                                                    - __io_read                                                                      ▒
                                                                                       - 32.49% xfs_file_read_iter                                                   ▒
                                                                                          - xfs_file_buffered_read                                                   ▒
                                                                                             - 31.04% filemap_read                                                   ▒
                                                                                                - 17.69% copy_page_to_iter
                                                                                                     17.45% _copy_to_iter                                            ▒
                                                                                                - 11.04% filemap_get_pages                                           ▒
                                                                                                   - filemap_get_read_batch                                          ▒
                                                                                                        3.55% xas_load                                               ▒
                                                                                                - 0.72% touch_atime                                                  ▒
                                                                                                     atime_needs_update                                              ▒
                                                                                             - 0.66% xfs_iunlock                                                     ▒
                                                                                                  up_read                                                            ▒
                                                                                               0.52% xfs_ilock_nowait                                                ▒
                                                                                   0.55% kiocb_done                                                                  ▒
                                                                              - 1.26% io_prep_rwv                                                                    ▒
                                                                                 - io_prep_rw                                                                        ▒
                                                                                    - 0.99% __io_import_iovec                                                        ▒
                                                                                       - __import_iovec                                                              ▒
                                                                                            0.66% copy_iovec_from_user                                               ▒
                                       - 3.79% virtio_blk_handle_request                                                                                             ▒
                                          - 2.36% virtio_blk_submit_multireq                                                                                         ▒
                                             - 1.43% blk_aio_preadv                                                                                                  ▒
                                                - blk_aio_prwv                                                                                                       ▒
                                                   - 0.55% blk_aio_get (inlined)                                                                                     ▒
                                                        0.53% qemu_aio_get                                                                                           ▒
                                             - 0.69% __GI___qsort_r                                                                                                  ▒
                                                - 0.67% msort_with_tmp.part.0                                                                                        ▒
                                                     0.53% msort_with_tmp.part.0                                                                                     ▒
                                            0.52% block_acct_start                                                                                                   ▒
                                       - 1.54% virtio_blk_submit_multireq                                                                                            ▒
                                          - 0.98% blk_aio_preadv                                                                                                     ▒
                                               blk_aio_prwv                                                                                                          ▒
                                 - 2.57% blkio_completion_fd_read                                                                                                    ▒
                                    - 0.94% blkioq_do_io                                                                                                             ▒
                                       - blkio::handling_result_with_value (inlined)                                                                                 ▒
                                          - blkio::blkioq_do_io::_$u7b$$u7b$closure$u7d$$u7d$::ha60b5b610ff6645a (inlined)                                           ▒
                                             - 0.81% blkio::Blkioq::do_io                                                                                            ▒
                                                  0.64% <blkio::drivers::iouring::IoUringQueue as blkio::Queue>::do_io                                               ▒
                                      0.74% qemu_aio_coroutine_enter
               - 1.32% g_main_context_check                                                                                                                          ▒
                    1.30% g_main_context_check_unlocked                                                                                                              ▒
            - 6.65% qemu_poll_ns                                                                                                                                     ▒
               - ppoll (inlined)                                                                                                                                     ▒
               - ppoll                                                                                                                                               ▒
                  - 6.59% entry_SYSCALL_64_after_hwframe                                                                                                             ▒
                     - do_syscall_64                                                                                                                                 ▒
                        - 6.55% __x64_sys_ppoll                                                                                                                      ▒
                           - 6.48% do_sys_poll                                                                                                                       ▒
                              - 1.54% sock_poll                                                                                                                      ▒
                                 - 1.30% udp_poll                                                                                                                    ▒
                                    - datagram_poll                                                                                                                  ▒
                                         0.60% add_wait_queue                                                                                                        ▒
                              - 1.52% poll_freewait                                                                                                                  ▒
                                   0.52% remove_wait_queue                                                                                                           ▒
                                1.21% fput                                                                                                                           ▒
                                0.76% fdget                                                                                                                          ▒
                                0.58% eventfd_poll                                                                                                                   ▒
            - 1.06% glib_pollfds_fill (inlined)                                                                                                                      ▒
               - 0.62% g_main_context_prepare                                                                                                                        ▒
                    0.61% g_main_context_prepare_unlocked
```

## 想要在 qemu + nixos 中使用，需要添加上这个 patch

1. 还需要 checkout 到 master
2. 不知道为什么 qemu 中 tests/set-completion-fd.c 测试就有问题，但是直接构建没有问题

```diff
diff --git a/build.sh b/build.sh
index bf3176ff51d9..0fe1c510833c 100755
--- a/build.sh
+++ b/build.sh
@@ -1,4 +1,4 @@
-#!/bin/sh
+#!/usr/bin/env bash
 set -e

 cd $(dirname $0)
diff --git a/containerized-build.sh b/containerized-build.sh
index 978514bd1710..51802167bd61 100755
--- a/containerized-build.sh
+++ b/containerized-build.sh
@@ -1,4 +1,4 @@
-#!/bin/sh
+#!/usr/bin/env bash
 cd "$(dirname $0)"

 # Build image, if it doesn't exist or is older than ./Containerfile
diff --git a/kcov-wrapper.sh b/kcov-wrapper.sh
index 9004fc576f3d..773a26a99cac 100755
--- a/kcov-wrapper.sh
+++ b/kcov-wrapper.sh
@@ -1,3 +1,3 @@
-#!/bin/sh
+#!/usr/bin/env bash
 test_name=$(basename $1_$3)
 exec kcov --exclude-path=/usr/include,/usr/src/libblkio/.cargo,/usr/src/libblkio/tests kcov-runs/$test_name "$@"
diff --git a/run-test-suites-in-vm.sh b/run-test-suites-in-vm.sh
index 8411332780cd..82b486f66437 100755
--- a/run-test-suites-in-vm.sh
+++ b/run-test-suites-in-vm.sh
@@ -1,4 +1,4 @@
-#!/bin/bash
+#!/usr/bin/env bash

 set -o errexit -o pipefail -o nounset
 export LANG=C
@@ -328,7 +328,7 @@ set +o errexit
         __ssh_batch driverctl unset-override 0000:00:05.0

         __log_info 'Running virtio-blk-vhost-user tests in guest...'
-        __ssh_batch /bin/bash -c '
+        __ssh_batch /usr/bin/env bash -c '
             set -e
             trap "kill -TERM %1; wait" EXIT
             qemu-storage-daemon \
diff --git a/src/cargo-build.sh b/src/cargo-build.sh
index 5c97ef3a8575..e0f3cdecc603 100755
--- a/src/cargo-build.sh
+++ b/src/cargo-build.sh
@@ -1,4 +1,4 @@
-#!/bin/bash
+#!/usr/bin/env bash
 set -e

 meson_debug="$1"
diff --git a/tests/blkio-bench.sh b/tests/blkio-bench.sh
index 8fac78b39052..a9d8bf800d98 100755
--- a/tests/blkio-bench.sh
+++ b/tests/blkio-bench.sh
@@ -1,4 +1,4 @@
-#!/bin/bash
+#!/usr/bin/env bash
 set -e

 FILE="blkio-bench-file"
diff --git a/tests/blkio-copy.sh b/tests/blkio-copy.sh
index b9a37b5121c6..569d33a4f763 100755
--- a/tests/blkio-copy.sh
+++ b/tests/blkio-copy.sh
@@ -1,4 +1,4 @@
-#!/bin/bash
+#!/usr/bin/env bash

 set -e

diff --git a/tests/blkio-info.sh b/tests/blkio-info.sh
index e3198ac59927..8b9c5998644d 100755
--- a/tests/blkio-info.sh
+++ b/tests/blkio-info.sh
@@ -1,4 +1,4 @@
-#!/bin/bash
+#!/usr/bin/env bash
 set -e

 FILE="blkio-info-file"
diff --git a/tests/props-blkio-info.sh b/tests/props-blkio-info.sh
index f62f79f94ec9..575b9bd9a0c8 100755
--- a/tests/props-blkio-info.sh
+++ b/tests/props-blkio-info.sh
@@ -1,4 +1,4 @@
-#!/bin/bash
+#!/usr/bin/env bash
 set -eu

 if [[ "$#" -le 2 || "$1" != "--path" || $(($# % 2)) -ne 0 ]]; then
diff --git a/tests/set-completion-fd.c b/tests/set-completion-fd.c
index b51232df018a..0eebc24664c0 100644
--- a/tests/set-completion-fd.c
+++ b/tests/set-completion-fd.c
@@ -94,7 +94,8 @@ int main(int argc, char **argv)
     char event_data[8];

     do {
-        read(completion_fd, event_data, sizeof(event_data));
+        int a = read(completion_fd, event_data, sizeof(event_data));
+        printf("[martins3:%s:%d] %d\n", __FUNCTION__, __LINE__, a);
         n += process_completions(q, &completion);
     } while (n < NUM_REQS);

--
2.47.2

```

## fio 居然有 libblkio 的后端

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
