# 分析常见的 vfs syscall 的流程

- do_sys_openat2
  - getname : read the file pathname from the process address space.
  - [ ] do_filp_open
  - get_unused_fd_flags :  find an empty slot in current->files->fd. The corresponding index (the new file descriptor) is stored in the fd local variable.
  - fd_install : insert the file to slot
