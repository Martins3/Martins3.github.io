# sed 

## todo 1
如果想要将其中含有 wp_page_reuse 的 backtrace 删掉

```txt
@[
    pte_mkdirty+101
    pte_mkdirty+101
    do_pte_missing+3008
    handle_mm_fault+2183
    do_user_addr_fault+504
    exc_page_fault+117
    asm_exc_page_fault+38
]: 46554
@[
    wp_page_reuse+203
    wp_page_reuse+203
    do_wp_page+1672
    handle_mm_fault+2657
    do_user_addr_fault+504
    exc_page_fault+117
    asm_exc_page_fault+38
]: 54266
```

这个不行:
```sh
sed -i '/@\[/,/]/ {/wp_page_reuse/d}' /home/martins3/core/vn/docs/kernel/mm-pgtable.md
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
