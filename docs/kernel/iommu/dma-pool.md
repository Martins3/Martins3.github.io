## dma pool 是什么
dma_pool_alloc

对应的 sysfs 接口 : /sys/devices/pci0000:00/0000:00:08.0/pools

就是 usb nvme wifi 喜欢用这个，其他都不用的:
```sh
for file in "/sys/devices/pci0000:00"/*; do
	a=${file##*/}
	lspci -s "$a"
	cat "$file"/pools || :
done
```

解释的很清楚了
https://stackoverflow.com/questions/60574054/why-do-we-need-dma-pool

```txt
@[
        dma_pool_alloc+5
        nvme_pci_setup_data_prp+233
        nvme_map_data+1034
        nvme_prep_rq.part.0+34
        nvme_queue_rqs+284
        blk_mq_dispatch_queue_requests+370
        blk_mq_flush_plug_list+136
        __blk_flush_plug+274
        blk_finish_plug+38
        xfs_buf_delwri_submit_nowait+306
        xfsaild_push+473
        xfsaild+216
        kthread+228
        ret_from_fork+417
        ret_from_fork_asm+26
]: 2
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
