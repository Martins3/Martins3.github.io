## Demos

这个目录现在有三份分层 demo：

- `chapter_demo.cu`
  - 目标：LDGSTS / `cp.async` 风格的 `global -> shared` 细粒度异步拷贝
  - 编译：`make ldgsts`
  - 运行环境：当前机器也能编译和运行
  - 说明：在 `sm_80+` 上走真正的异步拷贝路径；在 `sm_61` 上自动退化成同步拷贝，但代码结构保持一致

- `tma_bulk_demo_sm90.cu`
  - 目标：TMA 1D bulk copy（显式 `cuda::device::memcpy_async_tx`）
  - 编译：`make tma`
  - 运行环境：需要 `sm_90+`
  - 说明：这里故意只做 1D bulk copy，避免把 `CUtensorMap` 的 host 端编码也塞进最小 demo

- `stas_demo_sm90.cu`
  - 目标：STAS，把寄存器值异步写到另一个 block 的 distributed shared memory
  - 编译：`make stas`
  - 运行环境：需要 `sm_90+` 且支持 cluster launch
  - 说明：这是最小 cluster 教学骨架，重点是把 `map_shared_rank`、remote barrier、`st_async` 三者接起来

## Build

```bash
cd /home/martins3/data/vn/gpu/cuda/tutorial/04-async-copies
make ldgsts
make tma
make stas
```

## Why split them

这三种机制虽然都在同一章，但它们的硬件层次并不一样：

- LDGSTS：`sm_80+`
- TMA：`sm_90+`
- STAS：`sm_90+`，且依赖 cluster / DSMEM

所以把它们拆成三个独立 demo，会比硬塞进一个大文件更容易读，也更容易按硬件能力分别编译。
