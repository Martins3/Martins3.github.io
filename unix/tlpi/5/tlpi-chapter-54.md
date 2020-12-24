# Linux Programming Interface: Posix Shared Memory
> shmem_open 

In previous chapters, we looked at two techniques that allow unrelated processes
to share memory regions in order to perform IPC: System V shared memory
(Chapter 48) and shared file mappings (Section 49.4.2). Both of these techniques
have potential drawbacks:
- The System V shared memory model, which uses keys and identifiers, is not
consistent with the standard UNIX I/O model, which uses filenames and
descriptors. This difference means that we require an entirely new set of system
calls and commands for working with System V shared memory segments.
- Using a **shared file mapping** for IPC requires the creation of a disk file, even if
*we are not interested in having a persistent backing store for the shared region.
Aside from the inconvenience of needing to create the file, this technique
incurs some file I/O overhead.*
> System V shared memory is related to backing store ?

## 54.5 Comparisons Between Shared Memory APIs
