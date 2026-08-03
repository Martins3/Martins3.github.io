# generic/286: unwritten extent design note

## Current failure

`generic/286` fails in `test03`, which builds a file shaped like:

- hole
- multiple allocated-but-unwritten ranges
- multiple written ranges

The copy helper uses `SEEK_DATA` / `SEEK_HOLE` to clone only the data
extents. On simplefs, the destination file eventually inflates until
`ENOSPC`, and byte comparison fails.

## Current root cause

SimpleFS currently tracks `FALLOC_FL_KEEP_SIZE` preallocation with a single
inode-level range:

- `prealloc_block`
- `prealloc_len`

This model only describes one contiguous unwritten interval. It cannot
represent:

- `unwritten / written / unwritten`
- multiple disjoint unwritten regions after several writes into a large
  preallocated area

The failure is not only in the seek path. The more fundamental problem is in
`simplefs_prealloc_written()`:

1. a middle write lands inside the tracked prealloc interval
2. the function materializes the head of that interval with
   `simplefs_prealloc_materialize_range()`
3. only the tail remains tracked as unwritten

After this point, the head range becomes ordinary mapped zeroed data. That is
safe for plain reads, but no longer sparse from the point of view of
`SEEK_DATA` / `SEEK_HOLE`.

As a result, `seek_copy_test` copies ranges that should still behave like
holes/unwritten extents, and the destination grows until `ENOSPC`.

## Why local seek/read tweaks are insufficient

Treating the current inode-level prealloc range as hole in `read_iomap_begin()`
is not enough, because after the head range is materialized it is no longer
described by `prealloc_block` / `prealloc_len`. At that point only an
extent-level representation still knows whether the range is written or
unwritten.

## Minimal viable fix direction

Move unwritten state from inode-level bookkeeping to extent-level metadata.

That means:

1. Newly allocated `KEEP_SIZE` extents must be created as `unwritten`.
2. A write into an unwritten extent must:
   - split the extent if needed
   - convert only the written subrange to `written`
   - preserve unwritten state on the untouched head/tail pieces
3. Read / seek / fiemap paths must interpret extent state directly instead of
   relying on `prealloc_block` / `prealloc_len`.

## Code surfaces to touch

- `simplefs.h`
  - add an unwritten encoding for `struct simplefs_extent`
  - add helpers for masked length / state checks
- `simplefs_file.c`
  - `simplefs_get_block()`
  - `simplefs_fallocate_prealloc()`
  - `simplefs_write_iomap_begin()` / write conversion path
  - `simplefs_read_iomap_begin()`
  - `simplefs_fiemap()`
  - extent split/trim helpers used by punch/collapse/truncate
- extent normalization helpers must preserve unwritten state and avoid merging
  written with unwritten extents

## Non-goal for the first patch

Do not try to solve:

- `generic/017`
- mmap race cases
- full writeback integrity issues

The first success criterion is only:

- `generic/286` becomes correct
- existing known-good cases such as `040` and `740` keep passing


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
