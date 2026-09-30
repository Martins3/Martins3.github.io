# Next AI Draw.io 本地部署记录

本文记录 2026-08-25 在本机启用
[DayuanJiang/next-ai-draw-io](https://github.com/DayuanJiang/next-ai-draw-io)
并接入 DeepSeek V4 Flash 的过程。

## 最终状态

- 项目目录：`/home/martins3/data/next-ai-draw-io`
- 开发服务：<http://localhost:6002>
- AI provider：`deepseek`
- 模型：`deepseek-v4-flash`
- Node.js：`v22.23.1`
- npm：`10.9.8`
- Next.js：`16.2.6`
- 项目版本：`0.4.16`
- 项目工作区保持干净，`.env.local` 被 Git 忽略
- `.env.local` 权限为 `600`
- 已通过 DeepSeek 官方 API smoke test，返回模型为
  `deepseek-v4-flash`，测试响应为 `OK`
- 已验证 Web 首页返回 HTTP 200
- 已验证真实 API Key 没有出现在首页内容或 `/api/config` 响应中

API Key 不记录在本文中。真实值只保存在项目的 `.env.local` 中。

## 1. 核对官方使用方法

项目官方提供四种使用方式：

1. 在线演示站
2. 桌面应用
3. Docker
4. 本地运行 Next.js

本次使用第 4 种方式，因为仓库、Node.js、npm 和依赖都已经存在，不需要
重新安装软件。

参考资料：

- [项目中文 README](https://github.com/DayuanJiang/next-ai-draw-io/blob/main/docs/cn/README_CN.md)
- [AI provider 配置](https://github.com/DayuanJiang/next-ai-draw-io/blob/main/docs/cn/ai-providers.md)
- [DeepSeek V4 发布说明](https://api-docs.deepseek.com/news/news260424/)

## 2. 检查本机环境

执行：

```bash
cd /home/martins3/data/next-ai-draw-io

node --version
npm --version
docker --version
nix-shell --version
git status --short --branch
```

检查结果：

- Node.js、npm、Docker 和 Nix 均已存在
- 仓库已经 clone 到目标目录
- `node_modules` 已存在
- `npm ls --depth=0 --omit=optional` 返回成功
- Git 分支为 `main`，没有未提交修改
- `6002` 端口启动前没有被占用

因此没有执行 `git clone` 或 `npm install`。

如果以后在一台全新机器上部署，可以按官方步骤执行：

```bash
cd /home/martins3/data
git clone https://github.com/DayuanJiang/next-ai-draw-io
cd next-ai-draw-io
npm install
cp env.example .env.local
```

不要在已有 `.env.local` 时再次执行最后一条命令，否则会覆盖已有配置。

## 3. 第一次启动和问题定位

启动开发服务器：

```bash
cd /home/martins3/data/next-ai-draw-io
npm run dev
```

项目的 `dev` script 实际执行：

```bash
next dev --turbopack --port 6002
```

启动后检查首页：

```bash
curl --fail --silent --show-error --location \
	--output /dev/null \
	--write-out 'HTTP %{http_code}\n' \
	http://127.0.0.1:6002/
```

根路径首先返回语言跳转，跟随跳转后返回 HTTP 200。

当时 `.env.local` 使用 AWS Bedrock 和 Claude Sonnet，但本机没有 AWS CLI
配置、`~/.aws/config` 或 `~/.aws/credentials`。所以页面虽然能够打开，AI
请求却没有可用的服务端凭据。

## 4. 确认 DeepSeek 模型名称

DeepSeek 官方 V4 发布说明要求继续使用原 API 地址，只将模型名称改为：

```text
deepseek-v4-flash
```

项目当前代码也内置了这个模型：

- `lib/types/model-config.ts` 中 `SUGGESTED_MODELS.deepseek`
- `lib/ai-providers.ts` 中 `getAIModel()` 的 DeepSeek provider 分支

对于没有浏览器端自定义选择的请求，`hooks/use-model-config.ts` 会让客户端
使用服务端默认的 `AI_PROVIDER` 和 `AI_MODEL`。

## 5. 配置 `.env.local`

在 `/home/martins3/data/next-ai-draw-io/.env.local` 中设置：

```dotenv
AI_PROVIDER=deepseek
AI_MODEL=deepseek-v4-flash
DEEPSEEK_API_KEY=<从 DeepSeek 控制台创建的 API Key>
```

不要把真实 Key 写入本文、shell history、Git commit 或截图中。

收紧文件权限并确认 Git 会忽略它：

```bash
cd /home/martins3/data/next-ai-draw-io
chmod 600 .env.local
git check-ignore -v .env.local
git status --short --branch
```

预期 `.env.local` 权限为 `600`，`git check-ignore` 命中 `.env*.local` 规则，
Git 状态中不出现该文件。

## 6. 验证 DeepSeek API

下面的 smoke test 从 `.env.local` 读取 Key，不在命令行中写入真实 Key：

```bash
cd /home/martins3/data/next-ai-draw-io

set -a
source .env.local
set +a

curl --fail-with-body --silent --show-error \
	https://api.deepseek.com/v1/chat/completions \
	-H "Authorization: Bearer ${DEEPSEEK_API_KEY}" \
	-H 'Content-Type: application/json' \
	--data '{"model":"deepseek-v4-flash","messages":[{"role":"user","content":"Reply with exactly: OK"}],"max_tokens":16,"stream":false}' \
	| jq '{model, answer: .choices[0].message.content, error}'
```

本次实际结果：

```json
{
  "model": "deepseek-v4-flash",
  "answer": "OK",
  "error": null
}
```

这个请求会产生少量 API token 消耗。

## 7. 重启应用

修改 `.env.local` 后，在运行开发服务的终端按 `Ctrl-C` 停止旧进程，然后重新
启动：

```bash
cd /home/martins3/data/next-ai-draw-io
npm run dev
```

终端应显示：

```text
Local: http://localhost:6002
Ready
```

刷新 <http://localhost:6002> 后即可输入自然语言生成或修改 draw.io 图表。
例如：

```text
画一个 Linux 块设备 I/O 栈，从 read() 到 NVMe 控制器，标出 VFS、文件系统、
page cache、bio、blk-mq 和驱动之间的关系。
```

如果浏览器此前保存过自定义 provider，它会覆盖服务端默认值。此时在页面设置
中清除自定义模型选择，恢复使用 server default。

## 8. 配置行为说明

`/api/server-models` 返回空 provider 列表并不表示配置失败。单个
`AI_PROVIDER` + `AI_MODEL` 是服务端默认模型的兼容配置，只有显式配置
`AI_MODELS_CONFIG`、`ai-models.json`，或者在 `AI_MODEL` 中提供多个逗号分隔
模型时，模型才需要出现在服务端模型选择列表中。

应用实际处理聊天请求时，`app/api/chat/route.ts` 中 `handleChatRequest()` 会调用
`lib/ai-providers.ts` 中的 `getAIModel()`。没有浏览器端覆盖项时，后者读取：

```text
AI_PROVIDER=deepseek
AI_MODEL=deepseek-v4-flash
DEEPSEEK_API_KEY=...
```

## 9. 常用运维命令

启动：

```bash
cd /home/martins3/data/next-ai-draw-io
npm run dev
```

停止：在运行服务的终端按 `Ctrl-C`。

检查服务：

```bash
curl --fail --silent --show-error --location \
	--output /dev/null \
	--write-out 'HTTP %{http_code}\n' \
	http://127.0.0.1:6002/
```

检查监听端口：

```bash
ss -ltnp '( sport = :6002 )'
```

查看项目状态：

```bash
cd /home/martins3/data/next-ai-draw-io
git status --short --branch
```

## 10. 安全注意事项

1. API Key 曾通过聊天发送。如果聊天记录可能被共享或导出，应在 DeepSeek
   控制台轮换 Key，并同步更新 `.env.local`。
2. 不要提交 `.env.local`。
3. 不要在排障输出中打印完整环境变量。
4. 开发服务器适合本机使用，不应直接暴露到公网。
5. 如果需要团队共享或长期运行，应改用 production build、Docker 或受反向代理
   保护的部署，并配置访问控制。

## 11. 本次没有执行的操作

以下操作没有在本次验证中执行：

- 没有安装或升级 Node.js、npm、Docker、Nix 或 npm 包
- 没有执行 `git pull`
- 没有构建 production bundle
- 没有启动 Docker Compose
- 没有部署到 Vercel、Cloudflare 或公网服务器
- 没有修改项目源码

因此本文对本地开发模式和 DeepSeek API 接入给出的是已验证结果，其他部署方式
仍需单独验证。

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
