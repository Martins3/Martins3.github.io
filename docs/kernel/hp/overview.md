其实基本上已经成熟了

现在已经可以完善的做
nvme nic cpu memory 的热插拔，
所以，现在是需要的是 usb 热插拔

然后调查一下，除掉了这些热插拔，还有什么东西:

1. 直通到虚拟机的设备其实似乎可以热插拔的

可以想的角度:
1. 对于锁的实现
2. 从固件的角度 (ACPI)
3. pciehp 的角度

必须加上的东西，火影忍者的写轮眼的热插拔

## 思考一个问题，如何设计一个优秀的 hotplug 机制?

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
