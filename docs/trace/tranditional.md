# 传统的统计工具
- [ ] ps
- [ ] https://peteris.rocks/blog/htop/
- lsof

## [Meaning of every element of `htop`](https://peteris.rocks/blog/htop/)

> - the load number also includes processes in `uninterruptible` states which don't have much effect on CPU utilization.
> - `cat /dev/urandom > /dev/null &`
> - `mpstat 1`
> - `strace uptime 2>&1 | grep open`
> - Another name for a process is a task. The Linux kernel internally refers to processes as tasks. `htop` uses Tasks instead of Processes probably because it's shorter and saves some screen space.
> - `echo $!`
> - You can also see kernel threads with Shift+K
> - To toggle the visibility of threads, hit Shift+H on your keyboard
> - `od -c /proc/self/cmdline`
> - `tr '\0' '\n' < /proc/12503/cmdline`
> - If you hit `F5` in `htop`, you can see the process hierarchy. You can also use the `f` switch with `ps` : `ps f`. Or `pstree -a`
> - You can use the `id` command to find out the name for this user.
> - It turns out that id gets this information from the `/etc/passwd` and `/etc/group` files. `strace id  2>&1 | grep open`
> - TODO
