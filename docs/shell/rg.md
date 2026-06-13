# ripgrep
<!-- a45fce08-2af8-43f4-9424-91438a3da75f -->

- 只在某类文件里搜索
   - `rg -g '*.md' foo`
   - 例如只在 Markdown 文件里找 `foo`

2. 带行号搜索
   - `rg -n foo`
   - 默认通常就会带行号，显式写上更清楚

3. 忽略大小写
   - `rg -i foo`
   - 搜索 `foo`、`Foo`、`FOO`

4. 只匹配完整单词
   - `rg -w foo`
   - 避免把 `foobar` 也匹配进来

5. 同时搜索多个关键字
   - `rg 'foo|bar'`
   - 使用正则里的 `|` 表示或

8. 查看匹配前后文
   - `rg -C 2 panic`
   - 展示命中行前后各 2 行

9. 只显示命中的文件名
   - `rg -l TODO`
   - 适合先定位哪些文件包含目标内容

- 搜索隐藏文件: `rg -uu foo`
- 只看文件名而不是文件内容: `rg --files | rg foo`


也许下一步就是使用这个工具了:
https://github.com/tobi/qmd

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
