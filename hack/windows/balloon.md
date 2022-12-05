# 尝试分解一下
- https://stackoverflow.com/questions/6801008/globalmemorystatusex-win32
  - 获取 memory 的基本信息




- windows 的 malloc 直接会导致 memory 的使用量增加的
- windows 中，如果 free memory 分配完了，nalloc 就会失败，不会去压缩 page cache 的


# windows 内存管理
- https://learn.microsoft.com/en-us/troubleshoot/windows-server/performance/ram-virtual-memory-pagefile-management

In Windows systems, these paged out pages are stored in one or more files (Pagefile.sys files) in the root of a partition.
There can be one such file in each disk partition. The location and size of the page file is configured in **System Properties** (click **Advanced**, click **Performance**, and then click the **Settings** button).

- 在 windows 中如何查看 cache 的大小:
  - https://superuser.com/questions/793304/how-to-increase-swap-memory-in-windows
    - 输入 SystemPropertiesAdvanced.exe 即可
  - [ ] 存在疑惑的?
