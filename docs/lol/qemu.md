# 使用 qemu 构建双系统

- 忽然想到，其实可以这样，首先启动 Linux，使用 windows 所在的盘为一个盘，然后采用双系统的方法启动 windows 系统？
  - 似乎不行，因为这个盘里面没有 windows

- root 分区中到底有什么？
  - /boot 中内容

- 使用直通或者 /dev/nvme0n1 之类的方法，root 分区就在该 disk 上，
双系统无法启动哇。
