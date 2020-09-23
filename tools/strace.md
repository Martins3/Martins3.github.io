---
title: Strace
date: 2018-04-19 18:09:37
tags: kernel
---
strace is a powerful command line tool for debugging and trouble shooting programs in Unix-like operating systems such as Linux. It captures and records all system calls made by a process and the signals received by the process.
It displays the name of each system call together with its arguments enclosed in a parenthesis and its return value to standard error; you can optionally redirect it to a file as well.

In case a program crashes or behaves in a way not expected, you can go through its systems calls to get a clue of what exactly happened during its execution. As we will see later on, system calls can be categorized under different events: those relating to process management, those that take a file as an argument, those that involve networking, memory mapping, signals, IPC and also file descriptor related system calls.

[tutorial](https://www.tecmint.com/strace-commands-for-troubleshooting-and-debugging-linux/)
