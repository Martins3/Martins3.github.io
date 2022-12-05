# LWN 阅读笔记

## June

#### 23, 2022
https://lwn.net/Articles/898157/

- 介绍了 [WASM](https://webassembly.org/getting-started/developers-guide/) 流行之后，Python 社区推出的
  - 关联的视频 : https://www.youtube.com/watch?v=qKfkCY7cmBQ
  - 顺便尝试了一下: language/python/pyscript.html
- 又一个包管理器： https://flatpak.org/
- Fedora Project which is sponsored primarily by Red Hat
  - [ref](https://en.wikipedia.org/wiki/Fedora_Linux)

- LLVM CFI implementation
- NFS: the early years : 分析 NFS 从 stateless 到 stateful 的演进过程
- Disabling an extent optimization : 分析文件系统为了优化 sparse 文件，处理空洞的方式 fs cache 处理不同导致的 bug

## LWN 经典文章推荐

- https://lwn.net/Articles/453685/
- [Fun with NULL pointers, part 1](https://lwn.net/Articles/342330/)
- [Replacing x86 firmware with Linux and Go](https://lwn.net/Articles/738649/)
- [Writing an ACPI driver - an introduction](https://lwn.net/Articles/367630/)

## TODO
- https://lwn.net/Articles/899420/ : ubuntu bug 导致容器的问题，其中分析四种 fs
