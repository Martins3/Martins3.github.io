# selinux

## Links
- https://news.ycombinator.com/item?id=41946453
- https://github.com/SELinuxProject/selinux-notebook/releases
- https://github.blog/developer-skills/programming-languages-and-frameworks/introduction-to-selinux/


## 偶尔发现自己构建的内核在 qemu 中启动存在如下报错

```txt
# systemd[1]: systemd-journald-dev-log.socket: SO_PASSSEC failed: Operation not supported
```

```txt
`SO_PASSSEC` 是一个在 Linux 系统中用于 **UNIX 域套接字 (UNIX Domain Socket)** 的套接字选项 (Socket Option)。它的核心作用是**允许一个进程在接收数据时，一并获取发送方进程的“安全上下文 (Security Context)”**。

这个选项是 Linux 安全增强模块（如 SELinux）功能的一部分。

为了更好地理解，我们可以将其分解为以下几点：

### 1. 它是做什么的？ (What it does)

当你在一个 UNIX 域套接字上启用了 `SO_PASSSEC` 选项后，每当该套接字接收到来自另一个进程的消息时，操作系统内核不仅会传递消息数据本身，还会**附带一个特殊类型的辅助数据 (Ancillary Data)，其中包含了发送方进程的安全标签**。

这个安全标签就是所谓的“安全上下文”，它由 SELinux 等强制访问控制（MAC）系统定义和管理。

### 2. 它传递什么信息？ (What it passes)

`SO_PASSSEC` 传递的核心信息是**发送方进程的 SELinux 安全上下文**。

一个典型的 SELinux 上下文看起来像这样：
`user:role:type:level`

例如：`system_u:object_r:syslogd_t:s0`

这个标签详细地描述了进程的安全属性，包括它的用户、角色和类型。接收方进程可以根据这个标签来做出安全相关的决策。

### 3. 为什么这个功能很重要？ (Why it is important)

`SO_PASSSEC` 对于需要高度安全性的系统服务至关重要。它提供了一种可靠的方法来验证通信对端的身份和权限，而不仅仅是依赖于传统的用户ID（UID）或组ID（GID）。

主要用途包括：

* **增强的访问控制 (Enhanced Access Control):** 接收方服务（如 `systemd-journald` 日志服务）可以检查发送方（如某个应用）的 SELinux 上下文，然后根据预设的 SELinux 策略，决定是否接受其日志请求。例如，策略可以规定只有特定类型 (`type`) 的进程才能向日志系统写入日志。
* **详细的安全审计 (Detailed Security Auditing):** 日志服务可以将发送方的安全上下文与其日志消息一同记录下来。这为系统管理员提供了非常详细的审计追踪信息。当出现安全问题时，管理员不仅知道是哪个进程（PID）或用户（UID）产生了日志，还能精确地知道该进程当时所处的 SELinux 安全上下文，这对于安全分析和取证非常有价值。
* **防止欺骗 (Spoofing Prevention):** 由于安全上下文是由内核直接附加和验证的，因此它比进程自己声称的身份信息（如PID，因为PID可以被重用）要可靠得多，可以有效防止恶意进程伪装成合法服务进行通信。

### 4. 一个典型的应用场景：`systemd-journald`

`systemd-journald` 是 Linux 系统中负责收集和管理日志的核心服务。它通过一个 UNIX 域套接字（通常是 `/run/systemd/journal/socket`）来接收来自系统上其他进程的日志消息。

`journald` 会在其监听的套接字上启用 `SO_PASSSEC` 选项。这样，当一个应用向 `journald` 发送日志时，`journald` 不仅能收到日志内容，还能通过 `SO_PASSSEC` 获取到该应用进程的 SELinux 上下文。这使得 `journald` 能够：
1.  **根据 SELinux 策略过滤日志**：如果一个进程的上下文不被允许写入日志，`journald` 可以拒绝该消息。
2.  **为日志打上安全标签**：将进程的安全上下文存入日志元数据中，以便日后查询和分析。

如果你在一个没有启用 SELinux 的内核上运行 `systemd`，`journald` 在尝试启用 `SO_PASSSEC` 选项时就会失败，并产生你在上一个问题中提到的错误：`SO_PASSSEC failed: Operation not supported`。

### 总结

`SO_PASSSEC` 是一个专为基于 SELinux 的安全系统设计的底层机制。它通过在 UNIX 域套接字通信中可靠地传递进程的安全上下文，极大地增强了系统服务的访问控制能力和安全审计的深度，是构建高安全性 Linux 系统的重要一环。
```
## 检查 selinux 的状态
<!-- 51822bfd-c236-4da2-bebc-98e098a9d9e9 -->

getenforce

返回值含义：
- Enforcing：SELinux 已开启，并且在强制模式
- Permissive：SELinux 已开启，但只记录不拦截
- Disabled：SELinux 已关闭

## 基础实验：Unix 权限允许，SELinux 仍然拒绝
<!-- 8f9408fa-3178-49b3-8e67-616a9f665415 -->

2026-08-28 : emmmmm codex 快速搞了一个出来了，但是需要先看看 gihtub 的教程，获取一个大致的概念才可以

实验只关注三个基本概念：

- process domain：进程的 type，实验中是 `httpd_t`。
- object type：文件的 type，错误值是 `tmpfs_t`，Web 内容的正确值是 `httpd_sys_content_t`。
- allow rule：是否允许某个 domain 对某个 object type 执行某种操作。

### 准备实验

复制 `stat` 和 `cat`，并给它们 `httpd_exec_t` entrypoint 标签，便于用
`runcon` 进入 `httpd_t` domain：

```bash
sudo install -m 0755 /usr/bin/stat /usr/local/libexec/selinux-stat-demo.out
sudo install -m 0755 /usr/bin/cat /usr/local/libexec/selinux-cat-demo.out
sudo chcon -t httpd_exec_t /usr/local/libexec/selinux-stat-demo.out
sudo chcon -t httpd_exec_t /usr/local/libexec/selinux-cat-demo.out
```

创建测试文件，将 Unix mode 放宽到 `0777`，再故意设置错误的 SELinux type：

```bash
sudo mkdir -p /var/www/html
echo "SELinux basic demo" | sudo tee /var/www/html/selinux-demo.txt
sudo restorecon -Rv /var/www
sudo chmod 0777 /var/www/html/selinux-demo.txt
sudo chcon -t tmpfs_t /var/www/html/selinux-demo.txt
ls -lZ /var/www/html/selinux-demo.txt
```

此时能看到 Unix mode 是 `rwxrwxrwx`，但 type 是 `tmpfs_t`。

### 观察 SELinux 拒绝

```bash
sudo runcon system_u:system_r:httpd_t:s0 \
  /usr/local/libexec/selinux-stat-demo.out \
  /var/www/html/selinux-demo.txt
```

执行结果：

```txt
selinux-stat-demo.out: cannot statx '/var/www/html/selinux-demo.txt': Permission denied
```

查看 AVC：

```bash
sudo ausearch -m AVC -ts recent -i
```

```txt
denied { getattr } ...
scontext=system_u:system_r:httpd_t:s0
tcontext=unconfined_u:object_r:tmpfs_t:s0
tclass=file permissive=0
```

这证明 `chmod 0777` 只改变 DAC，不会绕过 SELinux MAC。

### 推荐修复：恢复正确标签

`/var/www` 已有标准 file-context 规则，因此通常应修正文件标签：

```bash
sudo restorecon -v /var/www/html/selinux-demo.txt
ls -lZ /var/www/html/selinux-demo.txt
sudo runcon system_u:system_r:httpd_t:s0 \
  /usr/local/libexec/selinux-stat-demo.out \
  /var/www/html/selinux-demo.txt
sudo runcon system_u:system_r:httpd_t:s0 \
  /usr/local/libexec/selinux-cat-demo.out \
  /var/www/html/selinux-demo.txt
```

`restorecon` 会将 type 改回 `httpd_sys_content_t`，然后 `stat` 和 `cat` 都成功。

## `.te` 策略文件怎么使用

`code/selinux-fd-share-getattr.te` 中的核心规则是：

```text
allow httpd_t tmpfs_t:file getattr;
```

其含义是：允许源 domain `httpd_t` 对目标 type `tmpfs_t` 的 `file`
执行 `getattr`。

`.te` 是策略源码，需要先编译为 `.pp` 策略包。源文件名需要与
`policy_module(selinux_fd_share_getattr, 1.0)` 中的模块名一致，所以复制时改用
underscore 文件名：

```bash
mkdir -p /tmp/selinux-getattr-policy
cp docs/kernel/security/code/selinux-fd-share-getattr.te \
  /tmp/selinux-getattr-policy/selinux_fd_share_getattr.te
make -C /tmp/selinux-getattr-policy \
  -f /usr/share/selinux/devel/Makefile \
  selinux_fd_share_getattr.pp
```

如果代码仓库不在 VM 内，可以先在 host 上传：

```bash
scp -P 52004 docs/kernel/security/code/selinux-fd-share-getattr.te \
  martins3@localhost:/tmp/selinux-fd-share-getattr.te
```

随后在 VM 内把上面的 `cp` 命令改为：

```bash
cp /tmp/selinux-fd-share-getattr.te \
  /tmp/selinux-getattr-policy/selinux_fd_share_getattr.te
```

安装并查看模块：

```bash
sudo semodule -i /tmp/selinux-getattr-policy/selinux_fd_share_getattr.pp
sudo semodule -l | grep '^selinux_fd_share_getattr'
```

把测试文件重新标成 `tmpfs_t`，先运行 `stat`：

```bash
sudo chcon -t tmpfs_t /var/www/html/selinux-demo.txt
sudo runcon system_u:system_r:httpd_t:s0 \
  /usr/local/libexec/selinux-stat-demo.out \
  /var/www/html/selinux-demo.txt
```

这次 `stat` 成功，可以看到文件大小、mode、UID/GID 和时间戳等元数据。
这就是 `getattr` 的作用。

然后用同一个 `httpd_t` domain 读取文件内容：

```bash
sudo runcon system_u:system_r:httpd_t:s0 \
  /usr/local/libexec/selinux-cat-demo.out \
  /var/www/html/selinux-demo.txt
```

`cat` 仍然失败：

```txt
selinux-cat-demo.out: /var/www/html/selinux-demo.txt: Permission denied
```

对应 AVC 从 `{ getattr }` 变成了 `{ read }`：

```txt
denied { read } ...
scontext=system_u:system_r:httpd_t:s0
tcontext=unconfined_u:object_r:tmpfs_t:s0
tclass=file permissive=0
```

因此 `getattr` 和 `read` 是两个独立权限：

| 实验状态 | `stat` 读元数据 | `cat` 读文件内容 |
|---|---|---|
| 没有自定义规则 | 被 `{ getattr }` 拒绝 | 被 `{ read }` 拒绝 |
| 只允许 `getattr` | 成功 | 仍被 `{ read }` 拒绝 |
| 用 `restorecon` 恢复正确 type | 成功 | 成功 |

卸载模块：

```bash
sudo semodule -r selinux_fd_share_getattr
```

这条 allow 会放行 `httpd_t` 对所有 `tmpfs_t` 文件的 `getattr`，范围比修正单个
文件标签更大。因此它适合学习 policy module 的编译和安装；在真实环境中，
如果 AVC 是由错误文件标签引起的，应优先使用 `restorecon`。

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
