# Gerrit

## 本地 Review Patch 指南
> [!NOTE]
> 参考神奇海螺的意见，有待验证

1. 直接 fetch 并 checkout patch

```bash
# 获取特定 patch（假设 patch number 是 1234）
git fetch origin refs/changes/34/1234/1 && git checkout FETCH_HEAD

# 查看代码
# ... 你的 review 操作 ...

# 返回原分支
git checkout -
```

2. 使用 git-review 工具（推荐）

```bash
# 安装 git-review
# Fedora: sudo dnf install git-review
# Ubuntu: sudo apt install git-review

# 列出待 review 的 patch
git review -l

# checkout 特定 patch 到本地分支
git review -d 1234
```

3. 配置 remote 简化操作

在 `.git/config` 中添加 fetch 规则：

```ini
[remote "origin"]
    url = <gerrit-url>
    fetch = +refs/heads/*:refs/remotes/origin/*
    fetch = +refs/changes/*:refs/remotes/origin/changes/*
```

然后可以直接：

```bash
git fetch origin
git checkout origin/changes/34/1234/1
```

4. 下载 patch 文件

```bash
# 从 Gerrit 网页获取 patch 的 URL，例如：
curl -o patch.diff https://gerrit.example.com/changes/1234/revisions/1/patch?zip
git apply patch.diff
```

5. 使用 tig 或 lazygit 等工具

```bash
# 安装后可以直接浏览 cherry-pick 的 patch
tig origin/changes/34/1234/1
```

## 再看看，再学学
一个 patch 可以看到一些链表，都是如何关联的:

- Relation chain     : gerrit 自动判断的
- Submitted together : gerrit 自动判断的，它们在同一个 topic 里 或者它们有依赖关系，Gerrit 判断必须一起合入
- Same topic         : 这是“同一个 topic 标签下的变更”，是逻辑分组，不一定有代码依赖。
- Merge conflicts

Relation chain
Submitted together
Same topic
Merge conflicts

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
