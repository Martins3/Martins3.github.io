# 使用 Github Pages 来搭建 Blog
之前也尝试将 blog 放到知乎或者简书上，但是这些平台和我的 github + vim 的笔记记录模式存在较大的冲突，
虽然可以获得不错的流量，但是最后还是放弃了。

而且之前还尝试过[很多酷炫的框架](https://github.com/myles/awesome-static-generators)，但是发现不够简洁，也都放弃了。
现在所有笔记都是放到上传到 Github 的[一个仓库](https://github.com/Martins3/Martins3.github.io) 中，这个页面也是其中[一个文件](https://github.com/Martins3/Martins3.github.io/blob/master/docs/setup-github-pages.md)生成的。
而配置只有两行:
```yml
title: Deep Dark Fantasy
theme: jekyll-theme-cayman
```

## 将仓库初始化为 blog
基础操作参考这里 : https://guides.github.com/features/pages/

- Github 提供了几个不错的主题，只需要在 `docs/_config.yml` 配置即可，我选择的是 : https://github.com/pages-themes/cayman
- 我建议将需要 docs/ 作为 blog 而不是整个仓库，如此，其他的位置都是草稿区，而 docs/ 中作为发布的

## 添加评论
在[静态网站上存在各种方法](https://darekkay.com/blog/static-site-comments/)添加评论，比如:
- [gitalk](https://github.com/gitalk/gitalk)

我使用的方案是 https://giscus.app/ , 按照其步骤，将其生成的代码粘贴到你的 blog 中，下面是我的例子:
```js
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
```

## 增加语法检查
使用 [lint-md](https://github.com/lint-md/lint-md) 和 [pre-commit](https://pre-commit.com/) 来构建语法检查，在这个仓库中，有三个文件和这个功能相关:
- .pre-commit-config.yaml : git commit 指向执行检查
- language/bash/hook/lint-md.sh : 其中一项检查是执行这个脚本
- .lintmdrc.json : lint-md 的执行参数
- 最后，执行 pre-commit install 来启用


https://github.com/DavidAnson/markdownlint 也是不错的

## 订阅
虽然本 blog 使用的 jellky 的主题，但是因为极简的配置，无法集成 [jekyll-feed](https://github.com/jekyll/jekyll-feed)

所以 Newsletter 来实现订阅:

https://martins3.substack.com

## 域名
不要使用域名，就使用 github.io 吧，见过太多由于没交钱最后网址失效的例子了。

## 写作技巧
- 语文教育中一直忽视了[基本的写作训练](https://threadreaderapp.com/thread/1554667451203276801.html)
- [Redhat 写作指南](https://stylepedia.net/style/)
- [写作的实用的10个技巧](https://threadreaderapp.com/thread/1554667451203276801.html)
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
