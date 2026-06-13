# github
## action

- https://upptime.js.org/docs/get-started

- https://github.blog/2021-04-15-work-with-github-actions-in-your-terminal-with-github-cli/
  - 使用 github action 来构造函数

- [ ] https://posthog.com/blog/automating-a-software-company-with-github-actions
  - 作者介绍他们公司如何使用 GitHub Actions，将各种开发步骤自动化，举了很多例子，演示了测试、代码格式化、构建、部署的配置文件应该怎么写。


### 一些有用的 action
- [issue 翻译](https://github.com/usthe/issues-translate-action)

## token
1. 创建 https://github.com/settings/tokens
2. 将 ~/.git-credentials 中原来的密码替换为 token:
```txt
https://martins3:token@github.com
```

## 目前为止，应该是最好的一个显示贡献的
https://honzaap.github.io/GithubCity/?name=martins3&year=2023

## 如何清空仓库
- https://stackoverflow.com/questions/4922104/is-it-possible-to-completely-empty-a-remote-git-repository

简单来说，你可以将任何仓库 push force 到仓库上，所以直接清空:
```sh
rm -rf .git
git init
git add .
git commit -m 'Initial commit'
git remote add origin git@github.com:XXX/xxx.git
git push --force --set-upstream origin master
```

## 下载一个 commit 对应的 patch
- https://stackoverflow.com/questions/21903805/how-to-download-a-single-commit-diff-from-github

很简单，直接在该 commit 的 url 的后面加上加上 .patch 就可以了

## gh-dash
https://github.com/dlvhdr/gh-dash

## github action yaml 的 lint
https://github.com/rhysd/actionlint

## 从 pr 下载 patch
https://stackoverflow.com/questions/6188591/download-github-pull-request-as-unified-diff

话说，存在更好的 review 代码的方法吗?


### Github 集成

通过 [github cli](https://github.com/cli/cli) 可以在终端上操作 github 上的 issue / pull request 等，
而通过 [octo.nvim](https://github.com/pwntester/octo.nvim) 可以将 github 进一步继承到 nvim 中。

1. 安装 github cli 参考[这里](https://github.com/cli/cli/blob/trunk/docs/install_linux.md)
2. 使用方法参考 octo.nvim 的 README.md

| 直接查看本项目中的 issue     |
| ---------------------------- |
| <img src="./img/octo.png" /> |


## 设置所有的 notification 为 read 的状态

TOKEN 和 github 的 token 相同:
```sh
curl -X PUT \
-H "Accept: application/vnd.github.v3+json" \
-H "Authorization: token $TOKEN" \
https://api.github.com/notifications
```

## 如何自动的和 github 沟通

安装:
```txt
curl -sL https://github.com/cli/cli/releases/download/v2.69.0/gh_2.69.0_linux_amd64.tar.gz -o gh.tar.gz
tar -xzf gh.tar.gz
cp /tmp/gh_2.69.0_linux_amd64/bin/gh ~/.local/bin/gh
```

配置:
```txt
gh auth login --hostname github.loongson.com
```

基本使用:
```txt
  gh pr list --repo kernel/mmoc-server

  # 查看某个 PR
  gh pr view 42 --repo kernel/mmoc-server

  # 发评论
  gh pr comment 42 --repo kernel/mmoc-server --body "测试评论"
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
