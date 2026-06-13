## 自动化控制理论
- https://github.com/TinyMPC/TinyMPC
- https://zhuanlan.zhihu.com/p/1914270546826339897?share_code=srHwb7MLFWch&utm_psn=1916864542807392630
  - Control Systems for Complete Idiots


## 就这个这个了，写的太棒了
https://space.bilibili.com/230105574

卡尔曼滤波器 : 为什么测量误差


## 类似需求的项目应该很多才对
https://kubernetes.io/docs/concepts/scheduling-eviction/

https://docs.ceph.com/en/reef/dev/osd_internals/mclock_wpq_cmp_study/

https://pve.proxmox.com/wiki/Ceph_mClock_Tuning

还有网络中的负载均衡。当然，还需要网络中的拥塞控制，按道理拥塞控制就最喜欢使用自动化控制理论了。
而且也是平衡多个 instance 直接的网络问题。

至少 CPU 的 scheduler 似乎根本没有自动化控制的思路，就是不看反馈的那种。

似乎 GPU 也是这个思路的: https://devblogs.microsoft.com/directx/hardware-accelerated-gpu-scheduling/

虽然 CPU 的 scheduler 是最复杂的，但是在存储栈和网络栈中，scheduler 是非常重要的:

似乎几个 block layer 的调度器都是分析如何合并 io ?
- 一个盘存在两个分区，如何保证在一个分区的 io 不会耗死掉另外一个分区的 io
- io 如何实现进程之间的平衡

网络上也是存在类似的问题，下载一个程序，然后整个机器几乎无法使用。

从这里切入分析 merge request

blk_account_io_merge_request

## 真正的 atuo tune ，没有想象的那么糟糕，但是还不错
https://github.com/oracle/bpftune 先跑起来再说了

这个项目是有意义的，无论是 ebpf 还是其他的什么

https://mp.weixin.qq.com/s/lQPg1FJYatlZedEfp-nDZw

这里还有深度学习的内容，有趣！

### 还有一个
https://lwn.net/Articles/998576/

## 有趣的
https://github.com/kubewharf/godel-scheduler


## 很好
现代控制理论怎么都是数学内容，太难太枯燥，怎么办？ - chitori kagome的回答 - 知乎
https://www.zhihu.com/question/423573211/answer/3412888030

1. 把教科书拿咸鱼上卖了
2. Dr.can和王开复（ACSheva）的视频，反复看
3. simulink，启动！
4. 去咸鱼上再把书买回来


## go scheduler 如何理解?
https://news.ycombinator.com/item?id=44022736

## 这个
https://www.databricks.com/blog/intelligent-kubernetes-load-balancing-databricks

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
