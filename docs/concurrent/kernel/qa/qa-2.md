## smp_load_acquire 和 smp_cond_load_acquire 的关系是什么?

只是一个封装而已，主要是 spinlock 和 scheduler 在使用
```c
#define smp_cond_load_acquire(ptr, cond_expr)
({
	typeof(ptr) __PTR = (ptr);
	__unqual_scalar_typeof(*ptr) VAL;
	for (;;) {
		VAL = smp_load_acquire(__PTR);
		if (cond_expr)
			break;
		__cmpwait_relaxed(__PTR, VAL);
	}
	(typeof(*ptr))VAL;
})
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
