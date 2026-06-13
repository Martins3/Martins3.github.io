# AI Skills 资源收集
https://agentskills.io/home

1. 让 AI 自己 git blame 检查东西
2. 构建一个沙盒，然后提供一个 root 权限吧
3. 自动分析 crash (的确可以)


## 下载所有的 kernel 邮件，然后用来分析
不容易的

## skill
https://github.com/travisvn/awesome-claude-skills

## 基本使用
https://moonshotai.github.io/kimi-cli/zh/customization/skills.html
需要额外注意的是:
- skill-creator
- flow 格式的 skill
- Agent 与子 Agent
```txt
~/.config/agents/skills/（推荐）
~/.kimi/skills/
~/.agents/skills

# 所以
ln -s /home/martins3/data/vn/.agents/  ~/.config/agents
```


https://github.com/obra/superpowers
https://github.com/ComposioHQ/awesome-claude-skills
https://github.com/openai/skills
看看这个项目基本是做什么的:
https://github.com/pydantic/monty

https://github.com/anthropics/skills


## 那么 spec drivern 是什么意思?

## 继续写 tutorial ，但是 tutorial 实际上是一个 claude 的 skills

忽然想到有趣想法，我发现现在 ai 的问题在于，遇到了问题实际上
不知道如何用更加复杂的方法来调试，例如:
1. 启动 qemu
2. 写内核模块测试，写系统调用测试
3. crash drgn
4. ...
5. 可以自动的分析 crash 了

现在 ai 会的方法还是当时我们看内核的第一步，也就是硬看代码，调用关系而已的

## skills 网站
- https://skills.sh/
	- https://skills.sh/vercel-labs/skills/find-skills
https://skillsmp.com/

那么继续这个东西:
https://manus.im/zh-cn/features/agent-skills?utm_source=nav_skillsmp

这里对比了，claude code ，manus 和 github skills

可以都调查一下

## 似乎 npx 系统就是最好的

## 无法读取 vnc 的屏幕
这个要求似乎太高了

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
