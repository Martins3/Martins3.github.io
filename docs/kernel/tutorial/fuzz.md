# fuzz
- 各种 fuzzer
  - https://www.csoonline.com/article/568135/9-top-fuzzing-tools-finding-the-weirdest-application-errors.html : 总结了一大堆
    - https://github.com/google/fuzzing https://github.com/kernelslacker/trinity https://github.com/google/fuzztest
  - https://github.com/intel/tsffs
  - https://github.com/google/honggfuzz
  - https://news.ycombinator.com/item?id=37989773 : CPU fuzzer，有趣的
  - https://www.youtube.com/watch?v=2PRGlLoUpLs : 对于 QEMU 的 fuzzing
    - https://mp.weixin.qq.com/s/RMbtfAE1Bh_cOAL8inVjIw
    - 其实 QEMU 中关于 fuzz 的内容还不少(直接搜索文件名
  - http://www.asuka39.top/article/ : 有一个 blog 是关于 kernel 的 fuzzing 的
  - https://github.com/u1f383/fuzzing-learning-in-30-days
  - https://github.com/fengjixuchui/FuzzingPaper
  - https://github.com/google/oss-fuzz
  - https://github.com/seemoo-lab/VirtFuzz
  - https://github.com/intel/kernel-fuzzer-for-xen-project
  - https://github.com/fuzzuf/fuzzuf

## QEMU fuzz
https://brieflyx.me/2023/2023-xz-salon/

https://news.ycombinator.com/item?id=41747979

## The Fuzzing Book
https://news.ycombinator.com/item?id=42756286

## 这里记录了一个 fuzzy 问题
https://github.com/nutanix/libvfio-user/blob/master/docs/testing.md

## 原来这个也是有 fuzzy 的
https://github.com/rust-vmm/vm-virtio?tab=readme-ov-file#fuzzing

## 硬件也可以 fuzz
https://github.com/revyos/xuantie-c900-bugs

## 原来 llvm 还有这个 fuzzer 啊
https://www.moritz.systems/blog/an-introduction-to-llvm-libfuzzer/


qemu 内部的 fuzz :
https://www.qemu.org/docs/master/devel/testing/fuzzing.html

https://github.com/0vercl0k/wtf

## fuzz 和 ai 结合

完全可以用这个作为 Guest 机器的负载来测试啊
- https://lore.kernel.org/all/20181228235106.okk3oastsnpxusxs@kshutemo-mobl1/T/#m78277db77a103fc113ce8c3759dfa6316fb8826f
- https://syzkaller.appspot.com/upstream

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
