## 为什么 napi enable 的时候需要屏蔽掉 softirq ?

理解一下:
```c
static void virtnet_napi_enable(struct virtqueue *vq, struct napi_struct *napi)
{
	napi_enable(napi);

	/* If all buffers were filled by other side before we napi_enabled, we
	 * won't get another interrupt, so process any outstanding packets now.
	 * Call local_bh_enable after to trigger softIRQ processing.
	 */
	local_bh_disable();
	virtqueue_napi_schedule(napi, vq);
	local_bh_enable();
}
```

```txt
@[
    __napi_schedule+5
    vring_interrupt+97
    __handle_irq_event_percpu+74
    handle_irq_event+62
    handle_edge_irq+157
    __common_interrupt+63
    common_interrupt+131
    asm_common_interrupt+38
    pv_native_safe_halt+15
    default_idle+9
    default_idle_call+46
    do_idle+499
    cpu_startup_entry+42
    start_secondary+286
    secondary_startup_64_no_verify+388
]: 1374
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
