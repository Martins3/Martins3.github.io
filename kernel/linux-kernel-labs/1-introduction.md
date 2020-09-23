# TODO

## Makefile, shell scripts and python scripts internals
1. img : 
    1. 100M myimg : what's it's purpose ?
    2. why, without special configuration, why already have vi ?
        3. I create a.md under /home/root, so where is a.md, in which disk ?
    3. what's realtion with bzimage disk1.img disk2.img and mydisk.img ?

2. dynamic kernel image : 128T
```
➜  .SpaceVim.d git:(master) ✗ l /proc/kcore 
.r-------- root root 128 TB Tue Mar 17 18:47:48 2020   kcore
```

2. change the login password ? set password, change user name

3. user mode kernel ?


  4. ldd3 : kernel debug chapter! read it.

5. https://en.wikipedia.org/wiki/Rootkit

> 6. The analysis of the kernel image is a method of static analysis. If we want to perform dynamic analysis (analyzing how the kernel runs, not only its static image) we can use /proc/kcore; 
> maybe, this should be done when the kernel version on host and qemu are consistent !

7. https://www.yoctoproject.org/docs/1.2/yocto-project-qs/yocto-project-qs.html


8. interesting !
```c
sudo mount -t ext4 -o loop core-image-minimal-qemux86.ext4 /tmp/tmp.takj147R7Q
```

9. make 和 make build 的各自的工作内容。

# Notes
4. we can  use gdb to **inspect the symbols** using the uncompressed kernel image. 

1. Using the dynamic image of the kernel is useful for detecting rootkits.

2. https://darkdust.net/files/GDB%20Cheat%20Sheet.pdf

To explore the contents of a 64-bit variable, use in the gdb console the command:
> x/gx & jiffies
If you wanted to display the contents of the 32-bit variable, you would use in the gdb console the command:
> x/wx & jiffies

3. 



