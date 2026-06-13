## Linux Capabilities 简述
<!-- b537edd1-253f-4fe8-bc4f-016303fad8b9 -->

man capabilities
https://manpages.debian.org/testing/manpages/capabilities.7.en.html

### 核心概念

Capabilities 将传统 root 的"所有特权"拆分为 41 个独立权限位。普通进程默认没有任何 capability。

```
CapInh:  Inherited      (继承自父进程)
CapPrm:  Permitted      (进程允许使用的 capabilities)
CapEff:  Effective      (当前实际生效的 capabilities)
CapBnd:  Bounding       (进程能获得的 capabilities 上限)
CapAmb:  Ambient        (可继承给子进程的 capabilities)
```

**实验验证当前进程:**

```txt
$ cat /proc/self/status | grep -E '^Cap'
CapInh:	0000000800000000
CapPrm:	0000000000000000
CapEff:	0000000000000000
CapBnd:	000001ffffffffff
CapAmb:	0000000000000000
```

- `CapEff=0`: 普通用户进程没有任何 effective capability
- `CapBnd=0x1ffffffffff`: bounding set 包含全部 41 个 capabilities（只是理论上允许获得）

### Capability 列表 (Linux 6.x)

| 编号 | 名称                   | 说明                            |
|------|------------------------|---------------------------------|
| 0    | cap_chown              | 修改文件 owner                  |
| 1    | cap_dac_override       | 绕过文件读/写/执行权限检查      |
| 2    | cap_dac_read_search    | 绕过文件读/目录搜索权限检查     |
| 3    | cap_fowner             | 绕过文件 owner 检查             |
| 4    | cap_fsetid             | 修改 setuid/setgid 位时不清理   |
| 5    | cap_kill               | 向任何进程发信号                |
| 6    | cap_setgid             | 修改进程 GID                    |
| 7    | cap_setuid             | 修改进程 UID                    |
| 8    | cap_setpcap            | 修改进程 capabilities           |
| 9    | cap_linux_immutable    | 修改 append-only/immutable 属性 |
| 10   | cap_net_bind_service   | 绑定到 <1024 的端口             |
| 11   | cap_net_broadcast      | (已废弃)                        |
| 12   | cap_net_admin          | 网络配置（接口/IP/路由等）      |
| 13   | cap_net_raw            | 使用 raw 和 packet sockets      |
| 14   | cap_ipc_lock           | 锁定共享内存                    |
| 15   | cap_ipc_owner          | 绕过 IPC ownership 检查         |
| 16   | cap_sys_module         | 加载/卸载内核模块               |
| 17   | cap_sys_rawio          | 原始 I/O (iopl/ioperm)          |
| 18   | cap_sys_chroot         | 使用 chroot                     |
| 19   | cap_sys_ptrace         | ptrace 任何进程                 |
| 20   | cap_sys_pacct          | 配置进程 accounting             |
| 21   | cap_sys_admin          | 各种管理操作（mount/swapon等）  |
| 22   | cap_sys_boot           | 重启/关机                       |
| 23   | cap_sys_nice           | 提升 nice 值                    |
| 24   | cap_sys_resource       | 突破资源限制                    |
| 25   | cap_sys_time           | 修改系统时间                    |
| 26   | cap_sys_tty_config     | 配置 TTY                        |
| 27   | cap_mknod              | 创建设备文件                    |
| 28   | cap_lease              | 在文件上建立租约                |
| 29   | cap_audit_write        | 写审计日志                      |
| 30   | cap_audit_control      | 配置审计子系统                  |
| 31   | cap_setfcap            | 设置文件 capabilities           |
| 32   | cap_mac_override       | 覆盖 MAC (如 SELinux)           |
| 33   | cap_mac_admin          | 配置 MAC                        |
| 34   | cap_syslog             | 操作 dmesg/syslog               |
| 35   | cap_wake_alarm         | 触发唤醒闹钟                    |
| 36   | cap_block_suspend      | 阻止系统休眠                    |
| 37   | cap_audit_read         | 读审计日志                      |
| 38   | cap_perfmon            | perf 性能监控                   |
| 39   | cap_bpf                | BPF 操作                        |
| 40   | cap_checkpoint_restore | CRIU checkpoint/restore         |

### File Capabilities 的三种模式

```sh
# +e : Effective (执行时自动加入 effective set)
# +p : Permitted (执行时加入 permitted set)
# +i : Inheritable (可继承)

sudo setcap cap_net_raw+ep /usr/bin/ping
```

### libcap 工具

`libcap` 提供用户空间操作 capabilities 的工具:

- `setcap`: 设置文件 capabilities
- `getcap`: 查看文件 capabilities
- `capsh`: capability shell，用于测试/调试

```txt
$ # tmpfs 上也正常
$ cp /usr/bin/ping /tmp/ping-tmpfs
$ sudo setcap cap_net_raw+ep /tmp/ping-tmpfs
$ getcap /tmp/ping-tmpfs
/tmp/ping-tmpfs cap_net_raw=ep
```

**capsh 实验:**

```txt
$ capsh --print | head -5
Current: cap_wake_alarm=i
Bounding set =cap_chown,cap_dac_override,...
Ambient set =

$ # 丢弃 CAP_NET_RAW 后，ping 仍然可以工作（因为用的是 SOCK_DGRAM）
$ sudo capsh --drop=cap_net_raw -- -c 'ping -c 1 127.0.0.1'
PING 127.0.0.1 (127.0.0.1) 56(84) bytes of data.
64 bytes from 127.0.0.1: icmp_seq=1 ttl=64 time=0.040 ms
```

## setuid

setuid 是类似的，只不过是继承传统的权限。

## 如何理解 can_do_hugetlb_shm

```c
static int can_do_hugetlb_shm(void)
{
	kgid_t shm_group;
	shm_group = make_kgid(&init_user_ns, sysctl_hugetlb_shm_group);
	return capable(CAP_IPC_LOCK) || in_group_p(shm_group);
}
```

创建 hugepage shared memory 需要满足 **任一条件**:
1. 进程有 `CAP_IPC_LOCK` capability
2. 进程在 `hugetlb_shm_group` 指定的组中

测试方法:
```txt
sudo sysctl vm.nr_hugepages=10
```

使用 ./code/test_hugetlb.c 测试
```bash
# 1. 默认状态，普通用户直接跑 -> 失败
./a.out
# shmget(SHM_HUGETLB) FAILED: Operation not permitted

sudo setcap cap_ipc_lock=ep ./a.out
./a.out
# shmget(SHM_HUGETLB) SUCCESS
```


## 调查一个 ping 的问题

ping 在使用我们构建的内核之后， 必须使用 root 了
6.6 内核:
```txt
  fping 10.0.0.2
fping: can't create socket (must run as root?)
~ 🌵
🤒  uname -r
6.6.0-87.0.0.82.oe2403.x86_64
~ 🌵
🧀  ping 10.0.0.2
PING 10.0.0.2 (10.0.0.2) 56(84) bytes of data.
64 bytes from 10.0.0.2: icmp_seq=1 ttl=64 time=0.438 ms
^C
--- 10.0.0.2 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 0.438/0.438/0.438/0.000 ms
```

使用 6.14.2 内核:
```txt
~ 🌵
🧀  fping 10.0.0.2

fping: can't create socket (must run as root?)
~ 🌵
🤒  ping 10.0.0.2
/usr/sbin/ping: socktype: SOCK_RAW
/usr/sbin/ping: socket: Operation not permitted
/usr/sbin/ping: => missing cap_net_raw+p capability or setuid?
```

sudo setcap cap_net_raw+ep $(realpath $(which ping))

```txt
🤒  /usr/sbin/ping 10.0.0.2
/usr/sbin/ping: socktype: SOCK_RAW
/usr/sbin/ping: socket: Operation not permitted
/usr/sbin/ping: => missing cap_net_raw+p capability or setuid?
```
遇到了一个完全相同的问题
https://www.reddit.com/r/openSUSE/comments/14neo41/pinging_from_a_base_user_wsl_tumbleweed/

结果为:
```txt
socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP) = -1 EACCES (Permission denied)
socket(AF_INET, SOCK_RAW, IPPROTO_ICMP) = -1 EPERM (Operation not permitted)
socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6) = -1 EACCES (Permission denied)
socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6) = -1 EPERM (Operation not permitted)
```
但是，如果使用 sudo ，那么又是可以的

啥也不做，切换到 fedora ，这个问题消失了，经过 ai 提示，
发现这两个函数总是会失败:
```txt
cp /usr/sbin/ping .
sudo setcap cap_net_raw+ep ping
```
然后发现是文件系统的原因。

CONFIG_EXT4_FS_POSIX_ACL=y
CONFIG_EXT4_FS_SECURITY=y

(这两个命令，如果是拷贝到 tmpfs 上，物理机可以，但是 fedora 虚拟机不行?)

## 为什么 id 总是相同的

```txt
uid=1000(martins3) gid=1000(martins3) groups=1000(martins3),10(wheel),987(docker)
```

它们并不总是相同，但你的系统恰好相同，是因为发行版用了 User Private Group（UPG） 机制。

根本原因：USERGROUPS_ENAB yes

```bash
  $ grep USERGROUPS_ENAB /etc/login.defs
  USERGROUPS_ENAB yes
```

这个配置在 /etc/login.defs 里，它告诉 useradd：

│ 创建用户时，自动创建一个同名、同号的私有组作为该用户的主组。

所以当你创建 martins3 用户时，系统同时做了两件事：
• 在 /etc/passwd 里写：martins3:x:1000:1000:...（uid=1000, gid=1000）
• 在 /etc/group 里写：martins3:x:1000:（gid=1000）

────────────────────────────────────────────────────────────────────────────────

为什么搞这个设计？

这是 Red Hat 在 RHEL 里推广的方案，现在 Fedora、CentOS、Rocky、Alma
以及很多其他发行版都沿用了。核心目的是让文件共享更方便。

传统 Unix 的默认行为是：所有新建用户的主组都是
users（gid=100）。这样两个人在同一个目录里干活时，文件默认对其他组员都是可读可写的，隐私性
很差。

UPG 方案下，每个人的主组是只有自己一个人的私有组。于是：
• 你创建的文件默认属组是你自己，别人看不到
• 当你真的想共享时，用 chgrp projectname file 或把目录设上 SGID 位即可

────────────────────────────────────────────────────────────────────────────────

反例：什么时候它们会不同？

┌────────────────────────────┬──────┬─────┬────────────────────────────────────────────┐
│ 场景                       │ uid  │ gid │ 说明                                       │
├────────────────────────────┼──────┼─────┼────────────────────────────────────────────┤
│ root                       │ 0    │ 0   │ 数字碰巧一样，但概念上是两个不同的命名空间 │
├────────────────────────────┼──────┼─────┼────────────────────────────────────────────┤
│ 系统用户 nginx             │ 991  │ 989 │ 创建时显式指定了不同 gid                   │
├────────────────────────────┼──────┼─────┼────────────────────────────────────────────┤
│ Debian 旧版本默认用户      │ 1000 │ 100 │ 早期 Debian 不设 UPG，主组默认是 users     │
├────────────────────────────┼──────┼─────┼────────────────────────────────────────────┤
│ usermod -g docker martins3 │ 1000 │ 987 │ 手动把主组改成 docker                      │
└────────────────────────────┴──────┴─────┴────────────────────────────────────────────┘

你可以验证 gid 可以独立修改：

```bash
  # 把主组临时切到 wheel
  $ newgrp wheel
  $ id
  uid=1000(martins3) gid=10(wheel) groups=10(wheel),1000(martins3),987(docker)
```

此时 uid 还是 1000，gid 变成了 10。

────────────────────────────────────────────────────────────────────────────────

总结

uid 和 gid 是两个完全独立的数字命名空间：
• uid 决定"你是谁"
• gid 决定"你创建文件时，默认属组是什么"

它们看起来一样，只是 useradd 为了方便，顺手分配了同一个号码。这是发行版策略，不是 Unix
内核的硬性规定。

## 参考资料
- 内核文档: [Credentials in Linux](https://www.kernel.org/doc/html/latest/security/credentials.html)
- capabilities 概述: <https://k3a.me/linux-capabilities-in-a-nutshell/>
- Rust caps 库: <https://github.com/lucab/caps-rs>
- HN 讨论: <https://news.ycombinator.com/item?id=45669142>

https://en.wikipedia.org/wiki/Universally_unique_identifier
https://en.wikipedia.org/wiki/User_identifier#Real_user_ID

- [Documentation](https://www.kernel.org/doc/html/latest/security/credentials.html)

https://news.ycombinator.com/item?id=45669142
https://github.com/lucab/caps-rs

- https://k3a.me/linux-capabilities-in-a-nutshell/
  - 一个具体的问题: https://stackoverflow.com/questions/45910849/shmget-operation-not-permitted

## TODO

忽然发现 root 不是最强的

例如这个问题，编译之后，先用 martins3 执行一次，然后用 root 执行就会有 permission 错误了
```txt
  // 初始化 shm 文件
  shm_fd = open(SHM_FILE, O_CREAT | O_RDWR, 0600);
  if (shm_fd < 0) {
    perror("open shm");
    exit(1);
  }
```

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
