# 2026-07-16 final generic xfstests results

## Environment

- VM: `yyds-fs`
- kernel: `7.1.2-00001-gd41bd6abfe34`
- filesystem: SimpleFS built from this worktree
- default test and scratch images: 2 GiB
- capacity rerun images: 24 GiB sparse files
- per-case timeout: 3600 seconds, except 551 and capacity reruns at 7200 seconds

System and root-user `systemd-tmpfiles-clean` timers, containerd/docker, and
other periodic maintenance timers were stopped before the runs. This isolates
the independently reproduced root-XFS reclaim lockdep issue documented in
`XFS_LOCKDEP_REPRO.md`.

## Base run

| Cases | PASS | NOTRUN | FAIL | TIMEOUT | Result root |
| --- | ---: | ---: | ---: | ---: | --- |
| 001-207 | 88 | 119 | 0 | 0 | `/var/tmp/simplefs-xfstests-final-unwritten-001-207` |
| 208 | 1 | 0 | 0 | 0 | `/home/martins3/data/simplefs-test-results/2026-07-16-generic-208` |
| 209-550 | 141 | 201 | 0 | 0 | `/var/tmp/simplefs-xfstests-final-unwritten-209-550` |
| 551 | 1 | 0 | 0 | 0 | `/var/tmp/simplefs-xfstests-final-unwritten-551` |
| 552-608 | 15 | 42 | 0 | 0 | `/var/tmp/simplefs-xfstests-final-unwritten-552-608` |
| 609 | 1 | 0 | 0 | 0 | `/var/tmp/simplefs-xfstests-final-unwritten-609` |
| 610-787 | 55 | 123 | 0 | 0 | `/var/tmp/simplefs-xfstests-final-unwritten-610-787` |
| Total | 302 | 485 | 0 | 0 | |

`generic/208` intentionally emits page-cache invalidation diagnostics and
filters them as part of the test. It passed after its fixed 200-second run;
the VM was force-rebooted immediately afterwards so those diagnostics could
not contaminate later dmesg checks.

The dm-error tests 338, 441, 442, 484, and 743 produced their expected
`dm-1` I/O errors. Generic 743 also exercised SimpleFS metadata-read `-EIO`
logging. These were fault-injection results, not unexplained test-device
errors; the VM was rebooted before subsequent clean-dmesg validation.

## NOTRUN corrections and capacity rerun

The base run's two `file_attr not built` skips were a harness deficiency.
`xfstests-full.sh` now builds `src/file_attr.out` with the guest-native compiler
and exposes the standard program name through a symlink. The corrected runs
showed:

- generic/772: `file_getattr not supported for regular files on simplefs`;
- generic/780: `file_getattr not supported for regular files on simplefs`.

Generic/641 now reaches the SimpleFS block-size check and reports
`Can't force 1024-byte file block size.` This is a legitimate skip because
the on-disk format has a fixed 4096-byte block size.

All eight capacity-only base skips were rerun with 24 GiB test and scratch
images under `/var/tmp/simplefs-xfstests-final-capacity`:

| Case | Corrected result |
| --- | --- |
| 038 | NOTRUN: FITRIM not supported |
| 312 | PASS |
| 590 | PASS |
| 620 | NOTRUN: dm-zero support is unavailable |
| 694 | PASS |
| 701 | PASS |
| 747 | PASS |
| 781 | NOTRUN: zoned loopback support is unavailable |

Five cases therefore move from NOTRUN to PASS. The final effective
disposition of all 787 generic cases is:

- PASS: 307
- legitimate NOTRUN: 480
- FAIL: 0
- TIMEOUT: 0

The remaining NOTRUN cases have explicit unsupported-feature or unavailable
test-environment reasons, including reflink/dedupe, encryption/verity, quota,
DAX, idmapped mounts, exchange-range/atomic-write operations, dm targets,
SCSI debug, zoned devices, FITRIM, and fixed-block-size constraints. NOTRUN
was not counted as PASS.

## High-value regression evidence

- generic/074 passed in about 80 seconds after speculative buffered
  preallocation was represented as an on-disk unwritten tail.
- generic/285 and 286 passed, including multi-range SEEK_DATA/SEEK_HOLE and
  dirty-page-in-unwritten-extent behavior.
- generic/299, 300, 521, 522, and 551 passed without data verification errors.
  Cases 521 and 522 each completed one million fsx operations.
- generic/551 completed all 100 random direct-AIO write/verify/truncate rounds.
  One round drove `MemAvailable` to about 171 MiB and performed multi-gigabyte
  readback, with no XFS lockdep warning or SimpleFS error after background
  trigger sources were stopped.
- generic/609 passed with no recursive inode-lock warning, validating the
  O_DSYNC/fsync preallocation cleanup fix.
- generic/095, 127, 363, 647, and 729 passed, covering the highest-risk
  mmap, buffered-I/O, DIO, GUP, punch, and fsx interactions.

The final module build, `bash -n simplefs/xfstests-full.sh`, and
`git diff --check` all completed successfully.

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
