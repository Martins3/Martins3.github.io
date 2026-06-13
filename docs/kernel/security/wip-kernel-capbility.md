## 对于这个问题理解的缺失已经影响了 Linux 的基本使用了

## capbility
- https://k3a.me/linux-capabilities-in-a-nutshell/
  - 一个具体的问题: https://stackoverflow.com/questions/45910849/shmget-operation-not-permitted


在 openEuler 2403 中安装 nix ，通过 nix 获取到了的 ping 之后
发现 ping 需要 sudo 才可以:
- https://superuser.com/questions/1035977/why-does-ping-require-the-setuid-bit

这里有两个问题:
1. 为什么 ping 需要 sudo ?
2. 除了上述特别的场景，一半情况是如何处理的，从而不使用 sudo ?


## 1
一个有趣的点，可以通过 /proc/sys/vm/hugetlb_shm_group

```c
static int can_do_hugetlb_shm(void)
{
	kgid_t shm_group;
	shm_group = make_kgid(&init_user_ns, sysctl_hugetlb_shm_group);
	return capable(CAP_IPC_LOCK) || in_group_p(shm_group);
}
```

## 2
cat /etc/group 中 可以看到 kvm 这个 group ，qemu 之所以可以访问 /dev/kvm
是由于访问了 /dev/kvm 得到的

## 多用户的管理之类的总结一下

```txt
useradd -m -G wheel -s /bin/bash shen # add one user
su - shen # change to user
```
很多教程都是说使用 -G sudo 来解决 shen 这个用户无法使用 sudo 的问题
但是在 centos 中测试实际发现是通过 wheel ，是发行版不同导致的吗?

sudo visudo 打开的这个文件的配置都是在给谁使用?
配置都是什么意思?

## user 和

## 主要的文件
kernel/cred.c

1. cred 处理的事情是 user group 的访问权限问题

- [Documentation](https://www.kernel.org/doc/html/latest/security/credentials.html)

#### (todo) setuid
man it


#### (todo) id
https://en.wikipedia.org/wiki/Universally_unique_identifier
https://en.wikipedia.org/wiki/User_identifier#Real_user_ID

## 常见问题
新安装 docker 的是时候经常 docker 权限问题，可以通过这个方法解决:

sudo usermod -a -G docker "$USER" 解决

之后就可以这样使用了:
docker run hello-world

但是这个方法需要 logout :

1. 对于 ssh 到机器上，将 ssh 退出，就可以
2. *似乎* 如果当时正在一个图形界面中，那么需要在界面中 logout 才可以

那么 docker run 修改了

## 有的解压目录中，可以得到如下的效果
```sh
sudo chmod -R u+rx,go-w everoute-log-collection-20250221-000144
```

## lwn
https://mp.weixin.qq.com/s/DH_bSrQ-Upfpckl9w1Muwg

## ping 在使用我们构建的内核之后， 必须使用 root 了
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
按道理，都是不该使用 root 吧

也许，等到 tar.sh 制作好了之后，用 full 来对比吧，
现在对于 capability 兴趣不大

## setfacl 是什么东西 ？
docs/snapshotting/handling-page-faults-on-snapshot-resume.md

sudo setfacl -m u:${USER}:rw /dev/userfaultfd

## 自己构建的 mini 内核，启动有这个问题
```txt
[   31.831707] systemd-journald[550]: Failed to set ACL on /var/log/journal/586bb79a6da64df386e725c241854aa2/user-1000.journal, ignoring: Operation not supported
```

## (misc) current_user() user.h
user tracking system ?
内核不仅仅需要管理 process, 而且需要管理用户和用户组:

那些内容需要内核出面:
1. 无处不在的权限检查
2. resource limit
3. ……

对于 current_user() 分析，发现在 task_struct 中间含有如下内容, 以前并没有注意到的内容
```c
	/* Process credentials: */

	/* Tracer's credentials at attach: */
	const struct cred __rcu		*ptracer_cred;

	/* Objective and real subjective task credentials (COW): */
	const struct cred __rcu		*real_cred;

	/* Effective (overridable) subjective task credentials (COW): */
	const struct cred __rcu		*cred;
```

跟踪 user 的结构体:
```c
/*
 * Some day this will be a full-fledged user tracking system..
 */
struct user_struct {
	refcount_t __count;	/* reference count */
	atomic_t processes;	/* How many processes does this user have? */
	atomic_t sigpending;	/* How many pending signals does this user have? */
```


## 我忽然发现 root 不是最强的

例如这个问题，编译之后，先用 martins3 执行一次，然后用 root 执行就会有 permission 错误了
```txt
  // 初始化 shm 文件
  shm_fd = open(SHM_FILE, O_CREAT | O_RDWR, 0600);
  if (shm_fd < 0) {
    perror("open shm");
    exit(1);
  }
```

## 文件系统和安全权限的关系
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

## libcap 是做什么的?


## 看看这个东西
https://news.ycombinator.com/item?id=45669142

https://github.com/lucab/caps-rs

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
