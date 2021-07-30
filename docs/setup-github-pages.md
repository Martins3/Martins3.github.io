# 如何使用两行代码搭建博客

## 第一行: 使用 Github 内置框架
Github 提供了几个不错的主题，只需要在 `docs/_config.yml` 配置即可

## 第二行: 搭建评论系统
可以使用 disqus，比如将下面的代码放到每一篇文章下面，其中三个位置需要调整
1. `this.page.url`
2. `this.page.identifier`
3. `s.src`: 这个需要在[你自己的 disqus 创建](https://disqus.com/admin/create/)

```html
<div id="disqus_thread"></div>
<script>
    /**
    *  RECOMMENDED CONFIGURATION VARIABLES: EDIT AND UNCOMMENT THE SECTION BELOW TO INSERT DYNAMIC VALUES FROM YOUR PLATFORM OR CMS.
    *  LEARN WHY DEFINING THESE VARIABLES IS IMPORTANT: https://disqus.com/admin/universalcode/#configuration-variables    */
    var disqus_config = function () {
    this.page.url = "https://martins3.github.io/";  // Replace PAGE_URL with your page's canonical URL variable
    this.page.identifier = 1; // Replace PAGE_IDENTIFIER with your page's unique identifier variable
    };
    (function() { // DON'T EDIT BELOW THIS LINE
    var d = document, s = d.createElement('script');
    s.src = 'https://https-martins3-github-io.disqus.com/embed.js';
    s.setAttribute('data-timestamp', +new Date());
    (d.head || d.body).appendChild(s);
    })();
</script>
<noscript>Please enable JavaScript to view the <a href="https://disqus.com/?ref_noscript">comments powered by Disqus.</a></noscript>
```

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
