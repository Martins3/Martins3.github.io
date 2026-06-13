- [ ] PA 中间 和 ucore 中间链接脚本

- https://eli.thegreenplace.net/tag/linkers-and-loaders :
    - https://eli.thegreenplace.net/2011/11/11/position-independent-code-pic-in-shared-libraries-on-x64
    - https://eli.thegreenplace.net/2011/11/03/position-independent-code-pic-in-shared-libraries/
    - https://eli.thegreenplace.net/2011/08/25/load-time-relocation-of-shared-libraries/
- [How linker works in llvm](https://www.youtube.com/watch?v=a5L66zguFe4)
- MaskRay 写的一堆小例子: https://github.com/MaskRay/ElfHacks
- https://www.lurklurk.org/linkers/linkers.html : 初学者入门

## 想不到 loader 可以直接加载 a.out
LD_DEBUG=all /nix/store/9xfad3b5z4y00mzmk2wnn4900q0qmxns-glibc-2.35-224/lib/ld-linux-x86-64.so.2 ./a.out
