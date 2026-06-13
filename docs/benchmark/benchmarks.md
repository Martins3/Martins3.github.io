# 性能基准测试工具

- https://github.com/martinus/nanobench
  - https://nanobench.ankerl.com/comparison.html#runtime : 还对比了其他的一堆 benchmark
- https://github.com/kdlucas/byte-unixbench


## 别人的测试
- [context switch](https://github.com/jimblandy/context-switch)

## 内存
- https://www.cs.virginia.edu/stream/
- https://github.com/raas/mbw

## 存储系统
- fio

## phoronix
```sh
curl -LO https://phoronix-test-suite.com/releases/phoronix-test-suite-10.4.0.tar.gz
tar -xvf phoronix-test-suite-10.4.0.tar.gz
cd phoronix-test-suite
sudo ./install-sh
phoronix-test-suite run pts/build-linux-kernel-1.9.1
```

## 综合工具
- https://github.com/masonr/yet-another-bench-script

## 内存测试
https://chipsandcheese.com/memory-bandwidth-data/

## CPU 微架构
https://github.com/clamchowder/Microbenchmarks


## 其他
https://github.com/wg/wrk
https://github.com/aquasecurity/kube-bench

##
https://github.com/ChipsandCheese/MemoryLatencyTest

## 清理 TLB 需要多久的时间?

## lmbench
https://github.com/intel/lmbench

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
