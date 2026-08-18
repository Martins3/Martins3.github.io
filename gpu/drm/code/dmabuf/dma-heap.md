• 它们的关系是：都能产出 dma-buf fd，但来源完全不同。

  可以直接这样区分：

  - dma_heap：分配一块新的内存，然后把这块新内存作为 dma-buf 返回。
  - udmabuf：不分配新内存，而是把你已经有的 memfd 页 pin 住，再包装成 dma-buf 返回。

  所以：

  - dma_heap 更像 allocator
  - udmabuf 更像 adapter/wrapper

  两者的输入输出对比：

  - dma_heap
      - 输入：你选一个 heap，例如 /dev/dma_heap/system
      - 输出：一个新分配出来的 dma-buf fd
  - udmabuf
      - 输入：一个已有的 memfd 和区间
      - 输出：基于这些已有页导出的 dma-buf fd

  内存所有权上也不一样：

  - dma_heap：内存是 heap 帮你新建的
  - udmabuf：内存原本就是你的 memfd
