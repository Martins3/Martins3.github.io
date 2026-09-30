# 性能基准测试工具

## 本仓库的测试
- [microbench/](microbench/README.md) : 本机 C++ CPU 微基准
- [gpu-microbench/](gpu-microbench/README.md) : GPU 微基准，与上面同方法同 CSV 约定
- [my-result.md](my-result.md) : 两者的测试记录
- [peak-flops.md](peak-flops.md) : 峰值算力怎么算，FLOP 怎么数，利用率怎么读

## 别人的
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
