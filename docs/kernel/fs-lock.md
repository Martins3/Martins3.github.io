# 记录一下 fs 的 lock 设计

## 现象 1

如果一个进程的 pwd 被删除了，在 /proc/10204/cwd 将是:
```txt
cwd -> '/root/gg (deleted)'
```
如果重新创建 /root/gg ，输出还是如此。
