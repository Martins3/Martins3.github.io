# Intel 核显的 syscall 抓取

本机 GPU 是 Intel UHD 770（PCI `0000:00:02.0`），内核驱动为 i915，
节点为 `/dev/dri/card1` 和 `/dev/dri/renderD128`。2026-10-19 在当前桌面上
全局采样约 15 秒，观察所有进程经过 `drm_ioctl()` 的调用。

最终样本共 **68,316 次 ioctl、4,090 次 EXECBUFFER2 提交**。
逐条 ENTER 数量与 BPF 内核聚合计数按请求号完全一致，EXIT 也为 68,316 条。
跟踪已退出，探针已卸载。

| 进程 | PID | ioctl 次数 | EXECBUFFER2 次数 |
| --- | --- | ---: | ---: |
| msedge | 3320364 | 28,656 | 1,684 |
| gnome-shell | 3309910 | 17,946 | 895 |
| msedge | 3320260 | 8,046 | 0 |
| WebKitWebProces | 362626 | 7,356 | 903 |
| clash-verge | 3310215 | 4,139 | 454 |
| ghostty | 648433 | 2,159 | 153 |
| Xwayland | 3311409 | 14 | 1 |

这是采样窗口内活跃的进程；打开 GPU fd 但未发出这些 ioctl 的进程不会出现在表中。
一个进程打开 GPU 并不等于它一直在提交渲染工作。例如第二个 Edge 进程的调用
涉及同步对象创建、导入、导出和传递，窗口内没有 EXECBUFFER2。

| ioctl（省略 DRM_IOCTL_ 前缀） | 次数 | 在做什么 |
| --- | ---: | --- |
| SYNCOBJ_WAIT | 17,665 | 等待或查询同步对象是否完成 |
| I915_GEM_MADVISE | 10,256 | 告诉驱动缓冲区是否仍需要保留，属于内存管理 |
| SYNCOBJ_CREATE / DESTROY | 7,563 / 7,598 | 创建、释放同步对象 |
| I915_GEM_EXECBUFFER2 | 4,090 | 提交 GPU 命令缓冲区及其引用的 buffer objects |
| I915_GET_RESET_STATS | 2,727 | 查询 GPU reset 状态；调用次数不代表 reset 次数 |
| SYNCOBJ_TRANSFER | 2,682 | 在同步对象或时间线间传递 fence |
| I915_GEM_SET_TILING | 1,841 | 设置缓冲区的平铺布局信息 |
| WAIT_VBLANK | 904 | 查询或等待显示垂直消隐 |
| MODE_ADDFB2 / MODE_CLOSEFB | 各 896 | 注册、关闭显示 framebuffer |
| MODE_ATOMIC | 895 | 原子提交显示状态，如 framebuffer、plane 和相关属性 |

例如 Edge 的一次真实调用，按抓到的参数写成便于阅读的形式：

```c
// PID 3320364，fd 19 对应 renderD128；省略未采集的字段。
ioctl(19, DRM_IOCTL_I915_GEM_EXECBUFFER2 /* 0x40406469 */, {
    .buffer_count = 91,
    .batch_start_offset = 0,
    .batch_len = 104424,
    .flags = 0xc1800,
    .rsvd1 = 1,  // 该 DRM fd 下的 context ID
}); // 返回 0
```

这表示提交一个指定长度为 104,424 字节的 batch，引用列表有 91 个 BO，
不是“执行 91 条指令”。`buffers_ptr` 指向 BO 描述数组，不是命令字节本身。
`0xc1800` 包含 `NO_RELOC`、`HANDLE_LUT`、`BATCH_FIRST`、`FENCE_ARRAY`：
提示免 relocation、使用对象数组索引、以第一个 BO 作为 batch，并携带同步 fence 数组。
context ID 属于对应 DRM 文件实例，不是跨进程的全局 GPU context 编号。
使用 context engine map 时，flags 中的 engine selector 是映射索引，不能直接当作引擎类型。

全部 EXECBUFFER2 返回成功；其驱动处理耗时中位数约 33 微秒，最大约 202 微秒。
这里的 `duration_us` 从 `drm_ioctl()` 入口计时到 syscall 返回，**不是 GPU 执行时间**。
另有 1,742 次 SYNCOBJ_WAIT 返回 `-62`（`ETIME`），表示等待条件在期限内未满足，
也可能是非阻塞查询，不能据此判定 GPU 故障。此脚本没有采集 wait 的 timeout 字段。

syscall 层能看见“提交、等待、分配、共享、显示更新”，但不能仅凭这些记录判断
某个 batch 在画文字、播放视频还是执行某个 shader。命令通常由用户态驱动写进
映射的 GEM 内存，再通过 EXECBUFFER2 提交；普通内存写入不会逐条产生 syscall。
进一步查看 `3DPRIMITIVE`、`PIPE_CONTROL` 或 shader 汇编需要读取和解码 batch 内容。
本次按要求只抓 syscall 及提交参数，没有把 ioctl 名称当成硬件指令。

抓取文件位于 `../reports/intel-20261019/`（reports 已由仓库忽略）：

- `ioctl.log`：最终原始记录及 BPF 聚合计数。
- `ioctl-decoded.log`：请求号转换成名称，保留原始请求号。
- `summary.tsv`：按进程、ioctl 分类的次数；`summary.json`：完整统计。
- `ioctl-names.txt`：从本机 libdrm 头文件展开得到的请求号和宏名映射。
- `tracer.log`：跟踪器诊断；最终运行无警告。
- `ioctl-first.log`、`ioctl-second.log`：调试阶段的样本，退出时未排空打印队列，
  不用于以上统计。最终脚本停止采集后额外留 2 秒排空队列。

从仓库根目录重新采集（约 15 秒，另留 2 秒排空；会覆盖最终日志）：

```bash
sudo env BPFTRACE_PERF_RB_PAGES=1024 bpftrace -B full \
    docs/trace/gpu/intel/ioctl.bt \
    > docs/trace/gpu/reports/intel-20261019/ioctl.log
gawk -f docs/trace/gpu/intel/decode.awk \
    docs/trace/gpu/reports/intel-20261019/ioctl-names.txt \
    docs/trace/gpu/reports/intel-20261019/ioctl.log \
    > docs/trace/gpu/reports/intel-20261019/ioctl-decoded.log
```

这两条命令只更新日志，不重新生成统计。脚本按本机 DRM major 226、minor 1/128
筛选实际 `struct file`，覆盖已有和采样期间新开的 fd，不依赖预先列出的 PID。
它不覆盖 dma-buf/sync_file fd 上的其他 ioctl、mmap、poll 等所有 GPU 相关 syscall。
ENTRY/EXIT 通过 TID 关联；`comm` 是线程名，PID 是进程 ID。
解码器对旧用户态结构大小不同的 ioctl 按 type/nr 回退识别，同时保留完整请求号。

接口定义可查看 `/usr/include/libdrm/i915_drm.h` 中的
`struct drm_i915_gem_execbuffer2`、`I915_EXEC_*`，以及
`/usr/include/libdrm/drm.h` 中的 `DRM_IOCTL_SYNCOBJ_*`。
背景说明见 [内核 i915 文档](https://docs.kernel.org/gpu/i915.html) 和
[DRM uAPI 文档](https://docs.kernel.org/gpu/driver-uapi.html)。

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
