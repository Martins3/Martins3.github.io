# QEMU 中的 map

> 从一个诡异的角度来分析 QEMU 中的源码。

既不会分析所有的种类的 map，也不会分析所有的使用位置，只是感觉老是遇到，总结一下。

## page lock 上锁的位置
```c
struct page_collection {
  struct GTree *tree;
  struct page_entry *max;
};
```

## 根据 Ram addr 找该 guest page 上关联的所有的 tb
page_find_alloc

## ???
```c
struct tcg_region_tree {
  QemuMutex lock;
  struct GTree *tree;
  /* padding to avoid false sharing is computed at run-time */
};
```

## 根据 physical address 计算出来 MemoryRegion
基本的调用流程如下：

- flatview_translate
  - flatview_do_translate
      - address_space_translate_internal
          - address_space_lookup_region
            - AddressSpaceDispatch::mru_section : 首先访问 mru(most recent used) 缓存，如果不命中，那么调用 phys_page_find 的 tree 中间寻找
            - phys_page_find

## 根据 virtual address 找到 tb
CPUState::tb_jmp_cache

## 根据 physical address 找到 tb
TBContext::htable
