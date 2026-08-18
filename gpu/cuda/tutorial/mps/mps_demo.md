# cuda mps
<!-- 20e9103d-b91f-4f94-aceb-fccd10eb561e -->

(有趣，原来是这样的)

## 实验 1：单进程 Baseline

```bash
./mps_demo.out 0 40000000
```

输出示例：
```
[Host] P0: launching kernel (iterations=40000000) ...
[Host] P0: wall-clock = 2419.61 ms, GPU time = 2419.59 ms
```

单个 kernel 在 GPU 上执行约 **2.42 秒**。

---

## 实验 2：Default Mode（无 MPS）双进程并发

确保 MPS 未运行：

```bash
ps aux | grep mps   # 应该没有 mps 进程
```

几乎同时启动两个进程：

```bash
./mps_demo.out 1 40000000 &
./mps_demo.out 2 40000000 &
wait
```

输出示例：
```
[Host] P2: wall-clock = 4975.57 ms, GPU time = 4975.57 ms
[Host] P1: wall-clock = 4975.54 ms, GPU time = 4975.48 ms
```

两个进程各花了约 **4.97 秒**，几乎是单进程的 **2 倍**。

**结论**：Default Mode 下，不同进程的 kernel 在 GPU 上是**串行执行**的。
一个进程执行完后，另一个进程才能开始。

---

## 实验 3：开启 MPS 后双进程并发

启动 MPS 服务：

```bash
nvidia-cuda-mps-control -d
```

再次同时启动两个进程：

```bash
./mps_demo.out 1 40000000 &
./mps_demo.out 2 40000000 &
wait
```

输出示例：
```
[Host] P2: wall-clock = 3928.30 ms, GPU time = 3928.29 ms
[Host] P1: wall-clock = 3931.44 ms, GPU time = 3929.43 ms
```

相比 Default Mode 的 4975 ms，MPS 下缩短到了约 **3930 ms**。

关键观察：
- P1 比 P2 晚启动约 1005 ms
- P1 只比 P2 晚完成约 1009 ms
- 这说明两个 kernel 在 GPU 上是**重叠执行**的，而非排队等待

**结论**：MPS 让多个进程的 kernel 真正共享 GPU 计算资源，实现并发执行。

---

## 停止 MPS

```bash
echo quit | nvidia-cuda-mps-control
```

---

## 原理总结

| 模式 | 多个进程能否同时访问 GPU | kernel 执行方式 | 典型场景 |
|---|---|---|---|
| **Default（无 MPS）** | 可以提交，但串行执行 | 时间片轮转，上下文切换 | 单进程独占 |
| **Default + MPS** | 可以提交，且并发执行 | 合并到同一上下文，SM 共享 | 多进程共享单卡 |
| **Exclusive Process** | 只有一个进程能访问 | 独占 | 需要强隔离 |

MPS 的本质：用一个 **MPS Server** 进程持有 GPU 上下文，所有客户端进程的 CUDA 调用都通过它代理，从而避免频繁的上下文切换开销。
