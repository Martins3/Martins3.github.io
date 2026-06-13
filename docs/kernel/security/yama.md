## 一个小的 fix
> [!NOTE]
> 参考神奇海螺的意见，有待验证

kernel.yama.ptrace_scope=2 将 Yama LSM 的 ptrace
访问控制设置为管理员模式（admin-only），其实际影响如下：

核心效果

普通用户完全失去 ptrace 能力，即使追踪的是自己的进程也不行。只有具备
CAP_SYS_PTRACE 能力的进程（通常是 root 或显式授权的二进制文件）才能使用
ptrace。

具体影响

┌───────────────────────────────────┬──────────────────────────┐
│ 场景                              │ 结果                     │
├───────────────────────────────────┼──────────────────────────┤
│ gdb -p <pid>                      │ 普通用户失败，权限被拒绝 │
├───────────────────────────────────┼──────────────────────────┤
│ strace -p <pid>                   │ 普通用户失败             │
├───────────────────────────────────┼──────────────────────────┤
│ 普通进程调试/注入其他进程         │ 被阻止                   │
├───────────────────────────────────┼──────────────────────────┤
│ root 或具有 CAP_SYS_PTRACE 的进程 │ 正常工作                 │
└───────────────────────────────────┴──────────────────────────┘

安全意义

这是最强悍的 ptrace 限制级别之一（某些内核版本还有 3=完全禁用）：

• 阻断凭证窃取：防止恶意程序通过 ptrace
  读取其他进程的内存（如提取密码、密钥）
• 阻断代码注入：阻止通过 ptrace 向其他进程写入 shellcode 或进行
  DLL/库注入
• 阻断部分逃逸攻击：限制容器内或受控环境中的进程横向移动

副作用

• 开发者和运维人员无法直接使用 gdb、strace 等工具调试普通进程
• 某些依赖 ptrace 的合法工具（如部分性能分析器、沙箱）需要显式授予
  CAP_SYS_PTRACE 能力才能工作

对比其他级别

• 0：经典模式，任何满足常规 UID 检查的进程都可 ptrace
• 1：受限模式，只允许父子关系或同一 UID 下显式授权的 ptrace（Ubuntu
  默认）
• 2：管理员模式，仅 CAP_SYS_PTRACE 可用
• 3：（如支持）完全禁用 ptrace

如果你的环境是生产服务器或高安全需求场景，2
是推荐配置；如果是开发调试环境，建议保持 1 或临时调整。

(但是，为什么 ptrace 是 yama 来管理的呢?)

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
