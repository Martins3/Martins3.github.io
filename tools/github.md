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
https://honzaap.github.io/GithubCity/?name=martins3&year=2022

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
