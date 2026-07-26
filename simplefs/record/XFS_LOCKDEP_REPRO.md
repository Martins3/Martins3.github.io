# XFS inode reclaim recursive-lock warning

## Result

On 2026-07-16 the `yyds-fs` VM reproduced an XFS lockdep warning without
loading SimpleFS.  The trigger is direct memory reclaim entered while a task
already holds an XFS directory ilock:

```text
(tmpfiles) is trying to acquire lock:
  (&xfs_dir_ilock_class), at xfs_icwalk_ag
but task is already holding lock:
  (&xfs_dir_ilock_class), at xfs_ilock_data_map_shared

xfs_dir_open
  xfs_dir3_data_readahead
    xfs_buf_alloc
      folio_alloc(GFP_KERNEL | __GFP_NOLOCKDEP | ...)
        try_to_free_pages
          shrink_slab
            super_cache_scan
              xfs_reclaim_inodes_nr
                xfs_icwalk_ag
                  xfs_reclaim_inode
                    xfs_ilock(ip, XFS_ILOCK_EXCL)
```

The running kernel was `7.1.2-00001-gd41bd6abfe34`, with
`CONFIG_PROVE_LOCKING=y`, `CONFIG_LOCKDEP=y`, a root XFS filesystem, 7.2 GiB
RAM, and no swap.

This is independent of SimpleFS.  `generic/551` happened to be a reliable
memory-pressure generator, while `containerd` or the periodic
`systemd-tmpfiles` job supplied the concurrent root-XFS directory access.

Three automatic trigger sources were observed across repeated `generic/551`
runs:

- `containerd`, shortly after the test started;
- the system-level `systemd-tmpfiles-clean.timer`;
- root's separate user-manager `systemd-tmpfiles-clean.timer`;
- the `polkit` `gmain` thread while adding an inotify watch to an XFS path.

The user-manager timer explains why stopping only the system timer did not
isolate the test.  A fourth background job, `dnf-makecache.timer`, started
`dnf5` during the same low-memory interval and invoked the OOM killer.  Its
stack did not contain the XFS lock recursion, but it was another uncontrolled
source of root-filesystem allocation and reclaim.

## Isolation validation

The final `generic/551` run stopped docker/containerd, the system maintenance
timers, and root's user-manager tmpfiles timer before starting the test. It
completed all 100 random direct-AIO write/verify/truncate rounds in about 25
minutes. The largest random round drove `MemAvailable` down to about 171 MiB
and performed several GiB of direct write and readback I/O, but produced no XFS
lockdep warning, hung-task report, OOM, or SimpleFS error.

The 2026-07-22 Phase 2 acceptance rerun additionally stopped polkit and moved
the xfstests work directory and loopback images from the XFS root filesystem to
a dedicated ext4 disk.  This removes the newly observed `gmain` accessor and
keeps the pressure workload itself off XFS.  In the final unified
2026-07-26 run, `generic/551` completed in about 26 minutes and PASSed without
an XFS lockdep warning, OOM, hung-task report, or SimpleFS error.  The complete
run covered `generic/001`--`generic/787` with PASS 432 / NOTRUN 355 / FAIL 0 /
TIMEOUT 0; its evidence is recorded in
`record/2026-07-20-phase2-jbd2-progress.md`.

This does not prove that memory pressure alone can never reach the XFS path;
it shows that the previously observed warning also needed a concurrent
root-XFS operation. Stopping only containerd was insufficient because the
system and root-user tmpfiles timers were independent trigger sources.

## Standalone reproducer

Run this only in a disposable VM.  It deliberately drives available memory
below 64 MiB and is likely to invoke the OOM killer on a differently sized or
busier system.

```bash
systemctl stop docker.socket docker.service containerd.service \
    systemd-tmpfiles-clean.timer dnf-makecache.timer polkit.service
XDG_RUNTIME_DIR=/run/user/0 \
    systemctl --user stop systemd-tmpfiles-clean.timer

# Populate XFS inode and directory caches before applying pressure.
find /usr /var -xdev -printf '' 2>/dev/null

python3 -c '
import time

chunks = []

def available_kb():
    with open("/proc/meminfo") as stream:
        for line in stream:
            if line.startswith("MemAvailable:"):
                return int(line.split()[1])
    raise RuntimeError("MemAvailable is missing")

while available_kb() > 65536:
    chunk = bytearray(8 * 1024 * 1024)
    for offset in range(0, len(chunk), 4096):
        chunk[offset] = 1
    chunks.append(chunk)

print(f"chunks={len(chunks)} available_kb={available_kb()}", flush=True)
time.sleep(300)
' >/var/tmp/xfs-lockdep-memory-pressure.log 2>&1 &
memory_pid=$!

for iteration in $(seq 1 300); do
    systemd-tmpfiles --clean >/dev/null 2>&1 || true
    if dmesg | tail -n 300 | grep -q \
            'WARNING: possible recursive locking detected'; then
        echo "reproduced at iteration $iteration"
        break
    fi
    sleep 0.1
done

kill "$memory_pid" 2>/dev/null || true
wait "$memory_pid" 2>/dev/null || true
dmesg | sed -n '/WARNING: possible recursive locking detected/,$p'
```

The local run reached about 44 MiB `MemAvailable` and reproduced on the 18th
`systemd-tmpfiles --clean` iteration.  Reboot the VM after reproduction because
lockdep disables further checking after a serious warning and the VM has been
under extreme memory pressure.

## Source-level analysis

The exact `xfs.ko` debug symbols map `xfs_icwalk_ag+0x48b` to the second ilock
in `xfs_reclaim_inode()` (`fs/xfs/xfs_icache.c:1044` in commit
`d41bd6abfe34`):

```c
/* Wait for an RCU inode-cache lookup to stop using the removed inode. */
xfs_ilock(ip, XFS_ILOCK_EXCL);
xfs_iunlock(ip, XFS_ILOCK_EXCL);
```

This is a blocking lock acquisition, not the earlier
`xfs_ilock_nowait()`.  Therefore the warning should not be dismissed merely
because reclaim initially uses a trylock.  The target inode has already been
removed from the per-AG radix tree, which greatly restricts possible races,
but the blocking lock is intentionally an RCU lookup barrier.

Two allocation paths produced the same recursion.  One begins in
`xfs_dir_open()`, which holds a shared directory ilock while issuing
best-effort block-zero readahead.  The other begins in `xfs_lookup()` and reads
a directory B-tree node through `xfs_da_read_buf()`, also while the directory
ilock is held.  For a page-sized XFS buffer,
`xfs_buf_alloc_backing_mem()` retains `__GFP_DIRECT_RECLAIM`; under pressure
either path can re-enter the XFS superblock shrinker and reach the blocking
reclaim ilock.

`__GFP_NOLOCKDEP` does not prevent this report.  It suppresses allocation's
filesystem-reclaim dependency annotation, but the later acquisition is still
an actual same-class recursive ilock operation observed by lockdep.

## Candidate fix directions

Clearing `__GFP_DIRECT_RECLAIM` when `XBF_READ_AHEAD` is set would make
metadata readahead genuinely non-blocking, but it fixes only the
`xfs_dir_open()` variant.  The observed `xfs_lookup()` variant uses an ordinary
metadata read, so a complete fix must also put non-transactional directory
buffer reads performed under the ilock into a scoped `memalloc_nofs_save()`
context, or otherwise allocate their backing memory without filesystem
reclaim.  This prevents the recursion instead of hiding it with a lockdep
subclass.

Adding a nesting annotation to the final reclaim ilock is not a sufficient
fix: inode reclaim can select an arbitrary inode, so there is no stable inode
ordering that the annotation could truthfully describe.

## Upstream status

The same reclaim pattern is tracked by syzbot as
`possible deadlock in xfs_icwalk_ag (3)` and remained open with priority high
when checked on 2026-07-16:

- https://syzkaller.appspot.com/bug?extid=789028412a4af61a2b61
- https://www.spinics.net/lists/linux-xfs/msg101411.html
- https://www.spinics.net/lists/linux-xfs/msg101413.html

The upstream report originally lacked a reproducer.  Later reports continued
to show the same XFS reclaim recursion on newer kernels.  Our reproducer is a
smaller operational trigger for the directory-ilock variant.

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
