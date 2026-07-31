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

## gerrit 对于多个 commit 提交的一般处理

在 Gerrit 里，多个 commit 就是多个 change，它们之间形成依赖链（relation chain)，按顺序逐个合入。这正是 Gerrit 处理"一个 feature 拆成多个原子改动"的标准方式：

基本做法

正常在本地写一串 commit（每个 commit 的 message 里都会被 commit-msg hook 加上各自的 Change-Id)，然后一次推：

```bash
  git push gerrit HEAD:refs/for/stable-6.2.0
```

Gerrit 会为链上每个 commit 各建一个 change，页面上能看到它们之间的父子依赖关系。review 也是逐个进行的——每个 change 独立 review、独立 +2。

合入顺序

- 必须先合入前面的 change，后面的才能合入（除非配置了 "submit whole topic")。
- 可以用 topic 把这一串 change 归成一组（push 时 HEAD:refs/for/stable-6.2.0%topic=my-feature，或推上去后在页面/命令行设置），方便整体查看；如果仓库开了
  submit-whole-topic，点一次 submit 会把整个 topic 按序一起合入。

修改链中间的 commit

这是 stacked changes 最麻烦的地方。比如要改第 2 个 commit:

```bash
  git rebase -i <链的起点前一个 commit>
  # 把要改的 commit 标记为 edit,停下来修改
  git add ...
  git commit --amend          # Change-Id 不变
  git rebase --continue       # 后面的 commit 被 rebase 到新基上,Change-Id 各自不变
  git push gerrit HEAD:refs/for/stable-6.2.0
```

因为每个 commit 的 Change-Id 都没变，push 后链上每个 change 各得到一个新 patch set，依赖关系自动重建。这正是 Change-Id 机制的价值：SHA 全变了，但逻辑身份不变。

实践建议

- 拆链的动机应该是"每个 commit 都可独立 review、独立通过测试"，而不是随意切。810 行插入的单个 change 其实已经在可接受范围内，别为了拆而拆。
- 链越长，维护成本越高（改第一个要重推整条链）。一般超过 5-6 个就要想想是不是该分几次合入。
- 如果后面的 commit 其实是修前面 commit 的问题，那就应该 amend 进前面的 commit，而不是叠在后面——回到上一条消息说的原则。

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
