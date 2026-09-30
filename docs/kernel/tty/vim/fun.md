
## zellij 居然还支持 web 模式啊
https://zellij.dev/tutorials/web-client/

## 哦，原来 asciiquarium 这么复杂

```txt
├─ptyxis─┬─ptyxis-agent─┬─zsh───.asciiquarium-w
│        │              ├─{dconf worker}
│        │              ├─{gdbus}
│        │              ├─{gmain}
│        │              └─{pool-spawner}
│        ├─{[pango] fontcon}
│        ├─{dconf worker}
│        ├─{gdbus}
│        ├─{gmain}
│        ├─{pool-spawner}
│        ├─2*[{ptyxis:disk$0}]
│        ├─{ptyxis:disk$1}
│        └─{ptyxis:disk$2}
```

而且在 asahi linux 上，这个会消耗 18% 的 CPU

不过，我没有理解，为什么会存在


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
