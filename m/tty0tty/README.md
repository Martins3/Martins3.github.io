如何测试，本来就提供了两个 demo :

- https://github.com/freemed/tty0tty/blob/master/pts/tty0tty.c

执行之后
```txt
 ./tty0tty
(/dev/pts/2) <=> (/dev/pts/3)
```
然后分别在两个窗口执行，然后可以发现两边都是互相输入都是可以回显的:
```txt
 socat -d -d STDIO /dev/pts/3
 socat -d -d STDIO /dev/pts/2
```

- https://github.com/freemed/tty0tty/blob/master/examples/tnt1_echo_tnt0.py

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
