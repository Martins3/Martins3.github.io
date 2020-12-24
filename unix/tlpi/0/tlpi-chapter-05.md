# Linux Programming Interface: Chapter 5

## 5.4 Relationship Between File Descriptors and Open Files
To understand what is going on, we need to examine three data structures
maintained by the kernel:
- the per-process file descriptor table;
- the system-wide table of open file descriptions; and
- the file system i-node table.

For each process, the kernel maintains a table of open file descriptors. Each entry in
this table records information about a single file descriptor, including:
- a set of flags controlling the operation of the file descriptor (there is just one such flag, the close-on-exec flag, which we describe in Section 27.4); and
- a reference to the open file description.

The kernel maintains a system-wide table of all open file descriptions. (This table is
sometimes referred to as the **open file table**, and its entries are sometimes called **open
file handles**.) An open file description stores all information relating to an open file,
including:
1. the current file offset (as updated by read() and write(), or explicitly modified
using lseek());
2. status flags specified when opening the file (i.e., the flags argument to open());
3. the file access mode (read-only, write-only, or read-write, as specified in open());
4. settings relating to signal-driven I/O (Section 63.3); and
5. a reference to the i-node object for this file.

We can draw a number of implications from the preceding discussion:
- Two different file descriptors that refer to the same open file description share
file offset value. Therefore, if the file offset is changed via one file descriptor
(as a consequence of calls to read(), write(), or lseek()), this change is visible
through the other file descriptor. This applies both when the two file descriptors belong to the same process and when they belong to different processes.
- Similar scope rules apply when retrieving and changing the open file status
flags (e.g., `O_APPEND`, `O_NONBLOCK`, and `O_ASYNC`) using the fcntl() F_GETFL and F_SETFL
operations.
- By contrast, the file descriptor flags (i.e., the close-on-exec flag) are private to
the process and file descriptor. Modifying these flags does not affect other file
descriptors in the same process or a different process.

## 5.5 Duplicating File Descriptors
> 介绍了几个 duplicate fd 的系统调用，以及使用的原因 : 将 stderr 和 stdout 指向同一个文件，将 stderr 关闭，然后 dup stdout，正好得到 stderr，如此，当 pipe 写入到 stderr 的实际上被指向到 stdout 中间。(也许如此吧!)
