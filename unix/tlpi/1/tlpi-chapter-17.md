# Linux Programming Interface: Chapter 17 : Access Control List
> Gerrit用户，我的伤心地，15章中间的内容也是有必要看一下的,用户 root 可以理解，但是用户组感觉从来没有见过，而且实际上自己的电脑上用户的数量多的一逼，远远不只有自己的几个用户

However, some applications need
finer control over the permissions granted to specific users and groups. To meet
this requirement, many UNIX systems implement an extension to the traditional
UNIX file permissions model known as access control lists (ACLs). ACLs allow file
permissions to be specified per user or per group, for an arbitrary number of users
and groups
> 为了实现更加精细化的权限控制

## 17.1 Overview
Each ACL entry consists of the following parts:
1. a tag type, which indicates whether this entry applies to a user, to a group, or to
some other category of user;
2. an optional tag qualifier, which identifies a specific user or group (i.e., a user ID
or a group ID); and
3. a permission set, which specifies the permissions (read, write, and execute) that
are granted by the entry.

The tag type has one of the following values:(细节)


A minimal ACL is one that is semantically equivalent to the traditional file permission set. It contains exactly three entries: one of each of the types ACL_USER_OBJ,
ACL_GROUP_OBJ, and ACL_OTHER. An extended ACL is one that additionally contains
ACL_USER, ACL_GROUP, and ACL_MASK entries.

One reason for drawing a distinction between minimal ACLs and extended
ACLs is that the latter provide a semantic extension to the traditional permissions
model. Another reason concerns the Linux implementation of ACLs. ACLs are
implemented as system extended attributes (Chapter 16). The extended attribute
used for maintaining a file access ACL is named system.posix_acl_access. This
extended attribute is required only if the file has an extended ACL. The permissions information for a minimal ACL can be (and is) stored in the traditional file
permission bits.
> 道出 ACL 扩展属性以及传统的文件属性的关系

## 17.2 ACL Permission-Checking Algorithm


## 17.3 Long and Short Text Forms for ACLs


## 17.5 The getfacl and setfacl Commands
> 演示了两个命令的使用，但是实测的部分命令无法复刻


## 17.6 Default ACLs and File Creation
> 当文件夹含有默认的ACL, 那么在该文件下创建的新文件、文件夹都是具有默认ACL

## 17.7 ACL Implementation Limits



## 17.8 The ACL API
> 介绍各种函数

