# acl
<!-- 1809f0fd-ead1-428c-8b1e-8ce24afb6edd -->

## 用户与用户组管理

wheel vs sudo 组是什么?

| 发行版                       | sudo 授权组 |
|------------------------------|-------------|
| Fedora/RHEL/CentOS/openEuler | `wheel`     |
| Debian/Ubuntu                | `sudo`      |

这是发行版差异，不是 Linux 内核的区别。

**实验验证:**

```txt
$ grep -E '^sudo:|^wheel:' /etc/group
wheel:x:10:martins3

$ sudo grep '%wheel' /etc/sudoers
%wheel	ALL=(ALL)	ALL
```

Fedora 默认给 `%wheel` 组 sudo 权限。`ALL=(ALL) ALL` 的含义：
- 第一个 `ALL`: 允许从所有主机登录的用户
- `(ALL)`: 可以切换为所有用户（包括 root）
- 最后一个 `ALL`: 可以执行所有命令

### 如何查看一个一个文件所属的 group
```sh
stat -c '%a %U %G' /dev/kvm
ls -la /dev/kvm
```

13900k 上执行的;
```txt
666 root kvm
crw-rw-rw- 10,232 root 11 Jun 15:06 /dev/kvm
```

kunpeng 上的:
```txt
660 root kvm
crw-rw---- 10,232 root 11 Jun 11:41 /dev/kvm
```

### 常用命令

```sh
# 创建用户并加入 wheel 组
sudo useradd -m -G wheel -s /bin/bash shen

# 修改用户密码
sudo passwd shen

# 切换用户
su - shen

# 查看当前用户 ID 和所属组
id
# uid=1000(martins3) gid=1000(martins3) groups=1000(martins3),10(wheel),987(docker)
```


```sh
sudo chmod -R u+rx,go-w everoute-log-collection-20250221-000144
```
这里 u+rx 是添加权限，不会移除所有者已有的写权限；而 go-w 是强制移除组和其他人的写权限，常用于防止非所有者误删或篡改日志文件。


## acl

### 问题
```bash
-rw-r----- 1 martins3 martins3 0 Jun 11 16:26 /tmp/acl_demo.txt
```

这个文件：
- `martins3` 能读写
- `martins3` 主组的人只能读
- 其他人什么都干不了

现在问题来了：如果我想让 `qemu` 用户能读写，同时让 `wheel` 组的人也能读，传统权限做不到。你只能：
1. 把 `qemu` 加入 `martins3` 组 -> 但这样 qemu 就拥有了 martins3 组的所有权限
2. 把文件属组改成 `wheel` -> 但原来的 `martins3` 组的人就访问不了了

### 解决方案
ACL 就是来解决这种**多用户、多组精细授权**的需求的。

```bash
$ touch /tmp/acl_demo.txt
$ chmod 640 /tmp/acl_demo.txt

$ setfacl -m u:qemu:rwx /tmp/acl_demo.txt   # 给 qemu 用户加 rwx
$ setfacl -m g:wheel:r /tmp/acl_demo.txt    # 给 wheel 组加 r

$ getfacl /tmp/acl_demo.txt
```

输出：

```
# file: tmp/acl_demo.txt
# owner: martins3
# group: martins3
user::rw-           <- 属主权限（传统 owner）
user:qemu:rwx       <- ACL 额外指定的用户
group::r--          <- 属组权限（传统 group）
group:wheel:r--     <- ACL 额外指定的组
mask::rwx           <- 有效权限掩码
other::---          <- 其他人权限（传统 other）
```

同时 `ls -l` 会多出一个 `@`
```bash
.rw-rwx---@ 0 martins3 11 Jun 16:32 /tmp/acl_demo.txt
```

这个 `@` 表示：该文件有 ACL 扩展，不要只看传统权限位，要用 `getfacl` 看完整权限。**

| 条目              | 含义                                                           |
|-------------------|----------------------------------------------------------------|
| `user::rw-`       | 文件属主（martins3）的权限。两个冒号中间为空，表示"属主本人"。 |
| `user:qemu:rwx`   | **指定用户** `qemu` 的权限。这是传统权限做不到的。             |
| `group::r--`      | 文件属组（martins3 组）的权限。两个冒号中间为空，表示"属组"。  |
| `group:wheel:r--` | **指定组** `wheel` 的权限。这也是传统权限做不到的。            |
| `mask::rwx`       | **权限掩码**。它限制了所有 ACL 用户和 ACL 组的**最大权限**。   |
| `other::---`      | 其他人的权限。对应传统权限的最后三位。                         |


所有**命名用户**（`user:xxx`）和**命名组**（`group:xxx`）的实际权限，最终是 `mask` 和各自条目的交集。

目录可以设**默认 ACL**（`default:` 前缀），它决定了**在该目录下新创建的文件/子目录自动继承什么 ACL**：

## 常用命令速查

```bash
# 查看 ACL
getfacl filename

# 给用户加权限
setfacl -m u:用户名:rwx filename

# 给组加权限
setfacl -m g:组名:rx filename

# 删除指定用户的 ACL
setfacl -x u:用户名 filename

# 清空所有 ACL，恢复传统权限
setfacl -b filename

# 给目录加默认 ACL（自动继承）
setfacl -m d:u:用户名:rwx 目录名

# 递归修改目录及所有内容的 ACL
setfacl -R -m u:用户名:rwx 目录名
```

## 实际案例
https://github.com/firecracker-microvm/firecracker/blob/main/docs/snapshotting/handling-page-faults-on-snapshot-resume.md 中提到:

```sh
sudo setfacl -m u:${USER}:rw /dev/userfaultfd
```
也就是给 acl 添加权限了。

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
