## grep 基本使用
<!-- f017b525-17be-4bf7-afd4-a73ace58a1f9 -->

- -o, --only-matching : 仅仅打印匹配的部分而不是该行
- grep -nr 'yourString.\*' . : rg 没有的时候用下, -r 表示 recursive
- grep -e aaa -e bbb : 同时搜索两个
- grep -n -C 2 something \* : -C 2 表示展示行数 -n 展示行号
- grep -r . /sys/module/zswap/parameters/
  - 打印的同时又展示出来数值
  - 或者 cd /sys/module/zswap/parameters/ && grep -r .
- grep -v -E "pciehp|vmwgfx"
  - 打印不用包含 pciehp|vmwgfx 的
  - -v 是 revert 的选择的意思

向前向后展示
  - grep -B 3 -A 2 foo README.txt
  - 其中 B 是 before , A 是 after 的意思

仅仅展示第一个 match -m 1
grep a todo.md -m 1

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
