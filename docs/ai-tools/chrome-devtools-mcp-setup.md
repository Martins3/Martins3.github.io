# Chrome DevTools MCP 配置指南

本文档介绍如何在 Kimi CLI 中配置 Chrome DevTools MCP，使 AI Agent 能够控制浏览器并访问需要登录的网站（如知乎）。

---

## 1. 环境要求

- **Node.js** v20.19 或更新版本
- **Chrome** 浏览器（当前稳定版或更新）
- **npm**（通常随 Node.js 安装）

检查 Node.js 版本：
```bash
node --version
```

---

## 2. 添加 MCP 服务器

### 方式 A：自动启动浏览器（推荐）

让 MCP 自动启动一个新的 Chrome 实例：

```bash
kimi mcp add --transport stdio chrome-devtools -- npx -y chrome-devtools-mcp@latest
```

### 方式 B：连接到已运行的浏览器（复用登录态）

如果你已经在 Chrome 中登录了知乎等网站，可以让 MCP 连接到这个已运行的实例：

```bash
kimi mcp add --transport stdio chrome-devtools -- npx -y chrome-devtools-mcp@latest --browser-url=http://127.0.0.1:9222
```

> **提示**：使用方式 B 时，先在浏览器中登录知乎/其他网站，然后启动 Kimi，这样 MCP 就可以使用你的登录会话。

---

## 3. 启动浏览器并启用远程调试

### macOS

```bash
/Applications/Google\ Chrome.app/Contents/MacOS/Google\ Chrome --remote-debugging-port=9222
```

### Linux

```bash
google-chrome --remote-debugging-port=9222
```

### Windows

```powershell
& "C:\Program Files\Google\Chrome\Application\chrome.exe" --remote-debugging-port=9222
```

---

## 4. 验证配置

### 列出所有 MCP 服务器

```bash
kimi mcp list
```

### 测试连接

```bash
kimi mcp test chrome-devtools
```

成功后会显示可用的工具列表，例如：

| 工具名 | 功能 |
|--------|------|
| `browser_navigate` | 导航到 URL |
| `browser_click` | 点击元素 |
| `browser_type` | 输入文本 |
| `browser_screenshot` | 截图 |
| `browser_eval` | 执行 JavaScript |

---

## 5. 使用示例

配置完成后，在 Kimi 中可以直接说：

```
帮我打开知乎并搜索"Python 教程"
```

Kimi 会：
1. 调用 `browser_navigate` 打开知乎
2. 如果需要登录，会使用你浏览器中已有的登录态
3. 执行搜索并返回结果

---

## 6. 高级配置

### 禁用数据收集（可选）

Google 默认会收集使用统计信息，可以禁用：

```bash
kimi mcp add --transport stdio chrome-devtools -- npx -y chrome-devtools-mcp@latest --no-usage-statistics
```

### 禁用性能报告的 CrUX API 调用

```bash
kimi mcp add --transport stdio chrome-devtools -- npx -y chrome-devtools-mcp@latest --no-performance-crux
```

---

## 7. 配置文件位置

MCP 配置存储在：

```
~/.kimi/mcp.json
```

你可以直接编辑这个文件来修改配置，例如：

```json
{
  "mcpServers": {
    "chrome-devtools": {
      "command": "npx",
      "args": ["-y", "chrome-devtools-mcp@latest", "--browser-url=http://127.0.0.1:9222"]
    }
  }
}
```

---

## 8. 管理 MCP 服务器

### 查看帮助

```bash
kimi mcp --help
```

### 移除服务器

```bash
kimi mcp remove chrome-devtools
```

### 重新授权（OAuth 服务器）

```bash
kimi mcp auth chrome-devtools
```

### 清除授权信息

```bash
kimi mcp reset-auth chrome-devtools
```

---

## 9. 故障排查

### 问题：连接失败

**检查项**：
1. Chrome 是否已启用远程调试（`--remote-debugging-port=9222`）
2. 端口是否正确（默认 9222）
3. 防火墙是否阻止了连接

**测试浏览器远程调试**：
```bash
curl http://127.0.0.1:9222/json/version
```

### 问题：登录态未复用

**原因**：方式 A（自动启动）会创建新的 Chrome 实例，不共享用户数据。

**解决**：使用方式 B，连接到已登录的浏览器实例。

### 问题：工具调用超时

部分操作（如截图、性能分析）可能需要较长时间，可以在调用时指定更长的超时时间。

---

## 10. 安全提醒

> **⚠️ 重要**：Chrome DevTools MCP 会暴露浏览器中的所有数据（Cookie、LocalStorage、页面内容等）给 AI，请确保：
>
> 1. 只在可信环境中使用
> 2. 避免在浏览器中打开敏感信息（银行、私人邮件等）
> 3. 使用完毕后及时关闭浏览器或断开 MCP 连接

## 参考链接

- [Chrome DevTools MCP GitHub](https://github.com/ChromeDevTools/chrome-devtools-mcp)
- [Kimi CLI MCP 文档](https://moonshotai.github.io/kimi-cli/zh/customization/mcp.md)
- [Model Context Protocol 官方文档](https://modelcontextprotocol.io/)

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
