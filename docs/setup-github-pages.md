# 使用 Github 记录笔记和搭建 blog

我的所有笔记都是放到上传到 Github 的[一个仓库](https://github.com/Martins3/Martins3.github.io) 中的，这个页面也是其中[一个文件](https://github.com/Martins3/Martins3.github.io/blob/master/docs/setup-github-pages.md)生成的。

## 动机

## 方法

#### setup up github pages
基础操作 : https://guides.github.com/features/pages/

Github 提供了几个不错的主题，只需要在 `docs/_config.yml` 配置即可

## 第二行: 搭建评论系统
但是更好的方案是 `https://utteranc.es/`, 按照其步骤，将其生成的代码(比如下面一行)粘贴到你的 blog 下即可。
```js
<script src="https://utteranc.es/client.js"
  repo="Martins3/Martins3.github.io"
  issue-term="url"
  theme="github-light"
  crossorigin="anonymous"
  async>
</script>
```
## 第三行 : 搭建 RSS 订阅
- [ ] TODO : [参考这里](https://dzhavat.github.io/2020/01/19/adding-an-rss-feed-to-github-pages.html) 似乎可行

- https://feed-me-up-scotty.vincenttunru.com/ 这个也似乎可行

如果你感觉操作有什么问题，在下面评论即可。
<script src="https://utteranc.es/client.js" repo="Martins3/Martins3.github.io" issue-term="url" theme="github-light" crossorigin="anonymous" async> </script>
