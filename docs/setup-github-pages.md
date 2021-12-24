# 使用 Github 记录笔记和搭建 blog

> 发表是最好的记忆
>
>   – 侯捷

我的所有笔记都是放到上传到 Github 的[一个仓库](https://github.com/Martins3/Martins3.github.io) 中的，这个页面也是其中[一个文件](https://github.com/Martins3/Martins3.github.io/blob/master/docs/setup-github-pages.md)生成的。

## 动机
- 很多小细节记录了，之后免得重新再次 google
- 记录笔记是输入的过程，写笔记是输出的过程，从而实现对于知识的完全理解。
- 总体来说，中文中的内容可看的比英语少很多，而且 CSDN 之类的毒瘤，鼓励抄袭盗版，搞中文社区更加是乌烟瘴气。改善这种情况，我认为每一个都从自己做起，尽量输出一些自己的原创内容。

## 方法
#### 将仓库初始化为 blog
基础操作 : https://guides.github.com/features/pages/

- Github 提供了几个不错的主题，只需要在 `docs/_config.yml` 配置即可
- 我选择的是 : https://github.com/pages-themes/cayman
- 我建议将需要 docs/ 作为 blog 而不是整个仓库，如此，其他的位置都是草稿区，而 docs/ 中作为发布的

#### 添加评论
在[静态网站上存在各种方法](https://darekkay.com/blog/static-site-comments/)

我使用的方案是 `https://giscus.app/`, 按照其步骤，将其生成的代码粘贴到你的 blog 中，下面是我的例子:
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
