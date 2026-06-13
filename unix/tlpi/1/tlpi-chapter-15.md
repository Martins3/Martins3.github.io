# Linux Programming Interface: Chapter 15 : File Attributes
We conclude the chapter with a discussion of i-node flags (also
known as ext2 extended file attributes),
which control various aspects of the treatment of files by the kernel.

> 我觉得Chapter 14 才是一举解决问题的根源

## 15.1 Retrieving File Information: stat()
> 描述一个struct stat

关键的两项:
```c
dev_t st_dev; /* IDs of device on which file resides */
ino_t st_ino; /* I-node number of file */
```

## 15.2 File Timestamps
> skip，过于无聊

## 15.3 File Ownership
> 分析了new file 的默认属性，以及用一个例子讲解chown fchown lchown 三个API

## 15.4 File Permissions
1. 读写权限在文件和目录中间表达的含义不同
2. 文件权限检查的逻辑是什么: 根据其逻辑，可以造成owner 无法访问，但是other 可以访问的怪事
> 到处都是的real 和 effective 是什么意思
3. access 判断权限的函数

As well as the 9 bits used for owner, group, and other permissions, the file permissions mask contains 3 additional bits, known as the set-user-ID (bit 04000), set-groupID (bit 02000), and sticky (bit 01000) bits.

 We have already discussed the use of the
set-user-ID and set-group-ID permission bits for creating privileged programs in
Section 9.3.

The set-group-ID bit also serves two other purposes that we describe
elsewhere: controlling the group ownership of new files created in a directory
mounted with the nogrpid option (Section 15.3.1), and enabling mandatory locking
on a file (Section 55.4). In the remainder of this section, we limit our discussion to
the use of the sticky bit.

However, these settings are modified by the file
mode creation mask, also known simply as the umask. The umask is a process
attribute that specifies which permission bits should always be turned off when new
files or directories are created by the process.

#### 15.5 I-node Flags (ext2 Extended File Attributes)


## 整理
group的作用是什么 https://jvns.ca/blog/2017/11/20/groups/
