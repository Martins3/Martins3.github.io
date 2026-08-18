# IRQ Trace for NVIDIA GPU

本文档记录这台 Fedora 42 宿主机上，CPU 如何接收并处理 NVIDIA GPU 中断。

观测时间: 2026-03-31
观测环境:

- 内核: `6.19.8-100.fc42.x86_64`
- GPU: `NVIDIA GeForce GTX 1060 3GB`
- 驱动: `nvidia`
- 工具: `bpftrace v0.24.1`

## 结论先行

这次实际 trace 到的中断处理路径是:

1. GPU 触发中断
2. CPU8 进入 NVIDIA 顶半部 `nvidia_isr`
3. 随后唤醒线程化中断下半部 `irq/139-nvidia`
4. 下半部在 `nvidia_isr_kthread_bh` 中继续处理

也就是说，这台机器上 NVIDIA 中断不是“全部在硬中断上下文里做完”，而是:

- 顶半部: 很短，在 hardirq 上下文执行
- 下半部: 放到线程化 IRQ kthread 里继续做

## 实际观测方法

运行:

```bash
cd /home/martins3/data/vn/gpu_demo/cuda
printf 'a\n' | sudo -S bash ./run_trace_nvidia_isr.sh
```

这个脚本会:

1. 挂上 [trace_nvidia_isr.bt](/home/martins3/data/vn/gpu_demo/cuda/trace_nvidia_isr.bt)
2. 运行 [vector_add.out](/home/martins3/data/vn/gpu_demo/cuda/vector_add.out)

输出:

- [trace_nvidia_isr.log](/home/martins3/data/vn/gpu_demo/cuda/trace_nvidia_isr.log)
- [trace_nvidia_isr.demo.log](/home/martins3/data/vn/gpu_demo/cuda/trace_nvidia_isr.demo.log)

## 这次实际跑到的关键日志

`vector_add.out` 运行成功:

```text
[OK] vector_add passed on device 0 with 1048576 elements
```

ISR trace 里出现了这些关键信息:

```text
NVIDIA_ISR cpu=8 comm=swapper/8 pid=0
NVIDIA_ISR_KTHREAD_BH cpu=8 comm=irq/139-nvidia pid=934574
...
NVIDIA_ISR cpu=8 comm=cuda00001400006 pid=934572
NVIDIA_ISR_KTHREAD_BH cpu=8 comm=irq/139-nvidia pid=934574
```

统计:

```text
@isr[8]: 26
@isr_kthread_bh[8]: 23
```

## 这些现象说明了什么

`NVIDIA_ISR cpu=8 comm=swapper/8 pid=0` 说明:

- CPU8 收到了 GPU 中断
- 进入的是硬中断上下文
- `pid=0` 是典型 hardirq 现场

`NVIDIA_ISR cpu=8 comm=cuda...` 说明:

- 有些中断是在用户线程正在 CPU8 上运行时到来的
- GPU 中断直接打断了那个正在跑的 CUDA 相关线程
- 顶半部仍然在同一个 CPU 上执行

`NVIDIA_ISR_KTHREAD_BH cpu=8 comm=irq/139-nvidia` 说明:

- 顶半部没有把所有工作都做完
- 驱动使用了线程化 IRQ 下半部
- 后续较重的处理被放到了 `irq/139-nvidia` 这个内核线程里

所以从 CPU 视角看，路径可以理解成:

1. PCIe 设备发出中断
2. APIC 把中断送到某个 CPU
3. Linux 进入该 CPU 上的硬中断入口
4. NVIDIA 顶半部 `nvidia_isr` 快速确认/应答
5. Linux 调度 `irq/139-nvidia` 线程
6. NVIDIA 下半部 `nvidia_isr_kthread_bh` 做后续处理
7. 等待中的 CUDA 路径再继续往前推进

## 和 memcpy demo 的对比

[memcpy_roundtrip.out](/home/martins3/data/vn/gpu_demo/cuda/memcpy_roundtrip.out) 那次没有直接抓到明显的 GPU 中断路径。

更合理的解释是:

- 那条 memcpy 路径更偏同步等待或轮询
- 或者它触发的中断形态不如 kernel launch 这样明显

但 [vector_add.out](/home/martins3/data/vn/gpu_demo/cuda/vector_add.out) 明确证明:

- 这台机器上的 GPU 中断确实会进入 `nvidia_isr`
- CPU 侧随后会通过 `irq/139-nvidia` 线程继续处理

## 当前脚本

- [trace_nvidia_isr.bt](/home/martins3/data/vn/gpu_demo/cuda/trace_nvidia_isr.bt)
- [run_trace_nvidia_isr.sh](/home/martins3/data/vn/gpu_demo/cuda/run_trace_nvidia_isr.sh)
- [trace_gpu_irq.bt](/home/martins3/data/vn/gpu_demo/cuda/trace_gpu_irq.bt)
- [run_trace_gpu_irq.sh](/home/martins3/data/vn/gpu_demo/cuda/run_trace_gpu_irq.sh)

其中 `trace_gpu_irq.bt` 是从 generic IRQ tracepoint 出发，`trace_nvidia_isr.bt` 是直接从 NVIDIA 驱动 ISR 符号出发。后者在这台机器上更有效。
