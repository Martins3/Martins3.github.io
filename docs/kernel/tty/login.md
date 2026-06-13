# login
## 两种
           ├─login(1152)───zsh(2205)
           ├─sshd(1022)───sshd-session(2006)───sshd-session(2037)───zsh(2044)───ps+

分别从两个地方登录

```txt
           ├─login(1184)───zsh(1411)
           ├─login(1185)───zsh(1707)
           ├─sshd(1061)───sshd-session(1195)───sshd-session(1225)───zsh(1232)───pstree(1903)
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
