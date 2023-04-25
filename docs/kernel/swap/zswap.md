## zswap
- http://www.wowotech.net/memory_management/zram.html : 就是这个吗 ?
  - https://github.com/maximumadmin/zramd

- [ ] 和 drivers/block/zram/ 是什么关系？

- [ ] zswap 压缩的内存还可以进一步的被 swap 出去的?

## 使用
cat /proc/swaps 如何调整一下 priority

存在 debugfs 吗？

如果在 centos 中打开?


`__zswap_pool_release` 中存在 `synchronize_rcu` 开始分析
