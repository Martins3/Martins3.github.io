## ACP
模式到底是什么?
https://moonshotai.github.io/kimi-cli/zh/customization/wire-mode.html

(这里，的 kimi wire 和 kimi acp 模式是什么关系?)

https://www.reddit.com/r/neovim/comments/1n1txun/support_for_agent_client_protocol_in/

类似的值得关注的插件: codecompanion.nvim

https://github.com/carlos-algms/agentic.nvim : 使用的人很少了

https://github.com/CopilotC-Nvim/CopilotChat.nvim

## 简单问了一下
```txt
一次标准 ACP 会话大概是：

    1. Client 启动 Agent 进程
    2. Client -> Agent: initialize
    3. Agent -> Client: initialize response
    4. Client -> Agent: session/new
    5. Agent -> Client: sessionId
    6. Client -> Agent: session/prompt
    7. Agent -> Client: session/update 若干次
    8. Agent -> Client: fs/read_text_file / terminal/create / request_permission 等
    9. Client -> Agent: 返回文件内容、命令输出、授权结果
    10. Agent -> Client: 更多 session/update
    11. Agent -> Client: session/prompt response，带 stopReason

stopReason 包括：

    end_turn
    max_tokens
    max_turn_requests
    refusal
    cancelled

七、一句话总结

ACP 定义的不是“一个大模型协议”，而是“编辑器和 coding agent 之间的 JSON-RPC 协议”。

它主要覆盖这些能力：

    初始化和能力协商
    认证
    session 生命周期
    用户 prompt
    agent 流式输出
    agent plan
    tool call 展示
    权限请求
    文件读写
    terminal 执行
    session mode/config
    MCP server 注入
    扩展方法

所以 codecompanion.nvim 这类插件做的事情，就是实现 ACP client，把这些协议映射到 Neovim 的 buffer、window、terminal、diff、selection、LSP
diagnostics 和用户交互上。
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
