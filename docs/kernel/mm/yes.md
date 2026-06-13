## folio_clear_swapbacked 和 __folio_clear_swapbacked 区别是什么?
<!-- 516b0482-f287-4980-9f7b-9926cec76524 -->

区别在于写 bit 是不是 atomic 的，这个差别非常重要。
这意味着，一个位置如果使用 __folio_clear_swapbacked ，那么当时就是保证
这个位置只有已经持有了什么 lock
```c
static __always_inline void folio_clear_swapbacked(struct folio *folio) {
  clear_bit(PG_swapbacked, folio_flags(folio, FOLIO_PF_NO_TAIL));
}

static __always_inline void __folio_clear_swapbacked(struct folio *folio) {
  __clear_bit(PG_swapbacked, folio_flags(folio, FOLIO_PF_NO_TAIL));
}
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
