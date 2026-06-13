- https://news.ycombinator.com/item?id=41946453

https://github.com/SELinuxProject/selinux-notebook/releases

github 写的 blog ，很好:
https://github.blog/developer-skills/programming-languages-and-frameworks/introduction-to-selinux/


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
