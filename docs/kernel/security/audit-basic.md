(kimi 写的，感觉非常合理了)

一、为什么需要审计功能

1. 合规要求

很多安全标准和法规强制要求系统具备不可抵赖的审计追踪能力，例如：
• PCI-DSS（支付卡行业数据安全标准）
• Common Criteria / CAPP
• ISO 27001、HIPAA
• 等保/分保等国内标准

这些标准要求：管理员的关键操作、敏感数据访问、权限变更等必须被记录，且记录本身要具备防篡改能力
。

2. 用户空间日志无法替代

syslog、journald 等日志是应用层/用户空间输出的，存在明显短板：
• 应用可以不记录、少记录，甚至伪造记录
• root 用户可以直接清空日志
• 被入侵后，攻击者通常会清理 /var/log/ 下的日志

Audit 的优势在于它是内核级的。事件在内核发生那一刻就被捕获，用户空间进程（包括
root）无法在事件产生后阻止其记录，只能事后删除日志文件（这本身也可以通过文件级审计监控）。

3. 安全溯源与取证

当发生入侵或异常行为时，需要回答：
• 谁（哪个用户/进程）？
• 在什么时间？
• 做了什么系统调用/访问了什么文件？
• 成功还是失败？

Audit 提供类似飞机"黑匣子"的详细记录，是 SOC（安全运营中心）和 SIEM 系统的重要数据源。

4. 调试疑难问题

某些偶发性故障（比如某个未知进程间歇性 kill 掉其他进程），通过 Audit 规则捕获 kill 相关
syscall，可以精准定位元凶。

────────────────────────────────────────────────────────────────────────────────

二、都可以审计哪些东西

Linux Audit 本质上审计的是事件（events），主要分为以下几类：

1. 系统调用（Syscall）

这是 Audit 最细粒度的能力。可以指定监控某个或某组 syscall：
• 文件类：openat, read, write, chmod, unlink, rename
• 进程类：fork, clone, execve, kill, exit
• 权限类：setuid, setgid, ptrace
• 网络类：socket, connect, bind

可以进一步过滤：只监控特定 UID、特定返回值（如失败的调用）、特定架构（x86_64 vs x86）。

2. 文件/目录监控（Watches）

对指定文件或目录进行访问审计：
• -w /etc/passwd -p rwa：监控对该文件的读、写、属性变更
• -w /etc/：递归监控整个目录树（遇到挂载点会停止，除非额外配置）
• 权限粒度：r（读）、w（写）、x（执行）、a（属性变更）

3. 认证与授权事件

• 用户登录/登出（LOGIN, USER_LOGIN）
• sudo、su 等权限提升
• SSH 密钥认证
• 账户变更（USER_ACCT, CRED_REFR）

4. 进程生命周期

• fork/clone 创建进程
• execve 执行新程序
• 进程退出（可记录退出码、信号等）

5. 安全子系统事件

• SELinux AVC（访问向量缓存）拒绝/允许事件
• Integrity Measurement Architecture (IMA) 完整性审计
• LSM（Linux Security Modules）相关日志

6. 用户空间主动上报

可信应用可以通过 libaudit 库主动向内核审计子系统发送自定义事件。例如 shadow-utils
在修改密码时会主动上报 USER_CHAUTHTOK。

7. 网络与块设备

• Netfilter 的 AUDIT target 可以记录特定网络包
• Device Mapper 的 DM_AUDIT 记录块设备映射事件

────────────────────────────────────────────────────────────────────────────────

三、具体如何实现

整个架构分为内核空间和用户空间两部分，通过 Netlink 套接字通信。

内核空间实现

1. 编译时支持

内核需要开启：

```
  CONFIG_AUDIT=y
  CONFIG_AUDITSYSCALL=y
  CONFIG_HAVE_ARCH_AUDITSYSCALL=y
```

2. 核心数据结构与上下文

• task_struct 中包含 struct audit_context *audit 指针
• 进程创建时：分配 audit context 并挂到 task 上
• syscall entry：填充基础信息（syscall 号、时间戳等），但不复制参数
• syscall 执行中：拦截 getname()、path_lookup()
  等内核内部路径解析，获取内核实际使用的、可信的文件名/ inode，而不是用户空间可能伪造的参数
• syscall exit：如果该调用被标记为
  "auditable"，则生成完整的审计记录，包含路径、inode、返回值、UID/GID、PID 等
• 进程退出：销毁 audit context

3. 规则匹配引擎：6 个过滤器列表

内核维护 6 个独立的规则列表，事件按顺序经过匹配：

┌────────────┬─────────────────────────────────────────────┐
│ 列表       │ 说明                                        │
├────────────┼─────────────────────────────────────────────┤
│ exclude    │ 全局排除特定消息类型或字段                  │
├────────────┼─────────────────────────────────────────────┤
│ task       │ 仅在 fork/clone 时检查，极少使用            │
├────────────┼─────────────────────────────────────────────┤
│ user       │ 过滤用户空间主动上报的事件                  │
├────────────┼─────────────────────────────────────────────┤
│ filesystem │ 文件系统类型相关过滤                        │
├────────────┼─────────────────────────────────────────────┤
│ exit       │ 最常用，所有 syscall 和文件监控请求在此评估 │
├────────────┼─────────────────────────────────────────────┤
│ io_uring   │ 监控 io_uring 底层 syscall                  │
└────────────┴─────────────────────────────────────────────┘

匹配规则采用 "First Match"：一旦命中规则，立即决定 always（记录）或 never（丢弃）。

4. Netlink 通信

内核通过 Netlink 将审计记录发送到用户空间：
• 如果有用户空间守护进程（auditd）监听，则通过 Netlink 交付
• 如果无守护进程，则回退到 printk 进入内核日志（dmesg/journal）

用户空间实现

1. auditd（审计守护进程）

• 通过 Netlink 接收内核事件
• 将事件写入 /var/log/audit/audit.log
• 管理日志轮转、磁盘空间监控
• 通过 plugin 接口 实时向外部程序分发事件（如远程日志服务器）

2. auditctl（规则管理）

动态加载、删除、查询内核审计规则。例如：

```bash
  # 监控 /etc/passwd 的读写属性变更
  auditctl -w /etc/passwd -p rwa -k passwd_watch

  # 监控所有失败的 setuid 调用
  auditctl -a always,exit -F arch=b64 -S setuid -F exit=-EPERM -k failed_setuid

  # 锁定规则（直到重启前无法修改）
  auditctl -e 2
```

注意：auditctl 加载的规则是内存中的，重启即丢失。

3. augenrules（持久化规则）

• 读取 /etc/audit/rules.d/*.rules
• 合并生成 /etc/audit/audit.rules
• 开机时通过 audit-rules.service 自动加载

4. ausearch / aureport（查询与报告）

```bash
  # 按 key 搜索
  ausearch -k passwd_watch -i

  # 查看今天所有失败 syscall
  ausearch -m SYSCALL -sv no --start today

  # 生成异常报告
  aureport -n
```

5. libaudit（开发库）

用户空间程序（如 login、sudo）可通过 libaudit 调用 audit_log_user_message() 等
API，主动向审计子系统注入事件。

完整数据流

```
  应用/用户空间          内核空间                     用户空间审计服务
      |                     |                               |
      | --- syscall --->    |                               |
      |                     | audit context 创建/填充       |
      |                     | 路径解析时记录 name/inode     |
      |                     | 规则匹配（6 个 filter list）  |
      |                     | 生成 audit record             |
      |                     | --- Netlink ----------------> auditd
      |                     |                               |--- 写入 /var/log/audit/audit.log
      |                     |                               |--- 通过 plugin 分发
      |                     |                               |
      | <--- 返回结果 ----  |                               |
```

性能与可靠性机制

1. Backlog 队列：内核维护一个事件队列（默认可能 64~8192 条），auditd
   消费不及时时会堆积。auditctl -s 可查看当前 backlog 和丢包数。
2. 速率限制：可配置每秒最大生成事件数，防止日志洪泛。
3. 尽早过滤：规则匹配在内核完成，未命中规则的事件不会走到 Netlink 传输，降低开销。
4. 规则锁定：auditctl -e 2
   可将规则设为不可变，防止攻击者加载空规则掩盖痕迹，只有重启才能解除。

总结

┌──────────┬─────────────────────────────────────────────────────────────────────────────────┐
│ 维度     │ 说明                                                                            │
├──────────┼─────────────────────────────────────────────────────────────────────────────────┤
│ 为什么   │ 合规要求、内核级防篡改日志、安全取证、疑难调试                                  │
├──────────┼─────────────────────────────────────────────────────────────────────────────────┤
│ 审计什么 │ syscall、文件访问、认证授权、进程生命周期、LSM/SELinux 事件、用户空间主动事件   │
├──────────┼─────────────────────────────────────────────────────────────────────────────────┤
│ 如何实现 │ 内核通过 6 个 filter list 匹配事件，在 task_struct 的 audit_context 中记录，经  │
│          │ Netlink 发送给 auditd 持久化；用户空间通过 auditctl 管理规则，ausearch 查询日志 │
└──────────┴─────────────────────────────────────────────────────────────────────────────────┘

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
