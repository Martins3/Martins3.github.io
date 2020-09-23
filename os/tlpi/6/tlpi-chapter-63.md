# Linux Programming Interface: Chapter 63 : Alternative IO Models
This chapter discusses three alternatives to the *conventional file I/O model* that we
have employed in most programs shown in this book:
1. I/O multiplexing (the select() and poll() system calls);
2. signal-driven I/O; and
3. the Linux-specific epoll API.

## 63.1 Overview
**Most** of the programs that we have presented so far in this book employ an I/O
model under which a process performs I/O on just one file descriptor at a time,
and each I/O system call blocks until the data is transferred.

We have already encountered two techniques that can be used to partially address
these needs: nonblocking I/O and the use of multiple processes or threads.

We described nonblocking I/O in some detail in Sections **5.9** and **44.9**.
