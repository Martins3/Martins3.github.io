## agent 到底是什么?

https://github.com/e2b-dev/awesome-ai-agents

AGENTS.md 每次 /init 出现的 AGENTS.md 是什么?

例如这里的，这里可以开启 agent 模式是什么意思
https://www.kimi.com/websites

再例如:
https://www.kimi.com/code/docs/kimi-cli/guides/interaction.html

## 类似的工具
https://github.com/Hmbown/DeepSeek-TUI

那么 codex 和 claude 都是开源的吗? 其实都是瞬间来搞定这个操作的。

## gemini 也是开源的吧?

好像也就是 claude 没有开源

## 哦，这个也是开源的
https://github.com/QwenLM/qwen-code

## kimi 对于 agent 的解释
这是一个常见的误解——**Kimi Agent 并不是运行在浏览器中的**，而是运行在**云端的沙盒环境（Sandbox）**里。

让我解释一下架构：

🏗️ 实际运行架构

```
你的浏览器/客户端
    ↓ 发送请求
Kimi 服务器（云端）
    ↓ 调度
沙盒环境（隔离的 Linux 容器）
    ├── Python/IPython 解释器
    ├── Node.js / 浏览器自动化工具
    ├── 文件系统 (/mnt/okcomputer/output)
    └── 网络访问能力
```

🔧 沙盒里有什么

| 组件 | 功能 |
|------|------|
| **Python 环境** | 运行 pandas、matplotlib、opencv 等，执行数据分析、图像处理 |
| **IPython** | 交互式代码执行，支持 Jupyter 风格的输出（图表直接显示） |
| **浏览器引擎** | 内置 Chromium，可以访问网页、点击、滚动、截图 |
| **文件系统** | 持久化存储，可以读写文件，保存生成的结果 |
| **Shell** | 执行系统命令（如 `git clone`、`npm install`） |
| **工具链** | 文档处理、图片生成、语音合成等 API |

🌐 网页访问是怎么实现的

当你让我"访问某个网页"时，实际上是：

1. 沙盒里的 **Chromium 浏览器** 打开该网页
2. 提取页面内容、元素列表返回给我
3. 我可以决定点击哪个元素、输入什么内容
4. 浏览器执行操作，返回新的页面状态

这和你在本地用 Selenium/Playwright 自动化浏览器是一样的原理。

🐍 代码执行是怎么实现的

当你让我"写个 Python 脚本分析数据"时：

1. 我把代码发送到沙盒的 **IPython 环境**
2. 代码在容器中执行（有 CPU、内存限制）
3. 结果（文本、图表、文件）返回给我
4. 我可以基于结果继续下一步操作

🔄 为什么感觉像"聊天"

前端界面看起来是个对话窗口，但**每次你发送消息，后端都是在调度这个沙盒环境**执行实际任务。对话历史只是状态管理，真正的计算都在沙盒里完成。

**简单说**：你不是在浏览器里和一个"纯 AI"聊天，而是在通过聊天界面**远程操控一个具备完整计算能力的云端工作环境**。

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
