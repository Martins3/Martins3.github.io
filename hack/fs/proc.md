# proc
- [ ] clear this document and pick useful info from plka/syn

- [ ] 内核中间只有 /proc/sys 的文档 https://www.kernel.org/doc/html/latest/admin-guide/sysctl/index.html

https://www.kernel.org/doc/html/latest/filesystems/proc.html

## /proc/devices
- fs/proc/devices.c
  - `chrdev_show`
  - `blkdev_show`

## /proc/kcore
2. dynamic kernel image : 128T
```
➜  .SpaceVim.d git:(master) ✗ l /proc/kcore
.r-------- root root 128 TB Tue Mar 17 18:47:48 2020   kcore
```
