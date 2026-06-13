# unix 文本处理

学一次，忘一次

- https://learnbyexample.github.io/cli_text_processing_coreutils/tr.html : 讲解各种 text processing 的事情，值得单独作为一个章节来分析。

## tr

- 大小写转换
```sh
tr '[:lower:]' '[:upper:]' <greeting.txt
```

- 过滤数字
echo toto.titi.3312.tata.2.abc.def | tr -d -c 0-9

## cut

- -f 后面跟着的参数类似 array 的 slice 的感觉
```sh
echo 'one;two;three;four' | cut -d';' --output-delimiter=$'\t' -f1,3-
```

## seq

类似 python 中的 range

seq 3

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
