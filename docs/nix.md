# Nix/NixOs 踩坑记录

最近时不时的在 hacknews 上看到 nix 相关的讨论:
- [Nixos-unstable’s iso_minimal.x86_64-linux is 100% reproducible!](https://news.ycombinator.com/item?id=27573393)
- [Will Nix Overtake Docker?](https://news.ycombinator.com/item?id=29387137)

忽然对于 Nix 有点兴趣，感觉自从用了 Ubuntu 之后，被各种 Linux Distribution 毒打的记忆逐渐模糊，现在想去尝试一下，
但是 Ian Henry 的[How to Learn Nix](https://ianthehenry.com/posts/how-to-learn-nix/) 写的好长啊，再次首先在 QEMU[^2] 上尝试一下效果，记录一踩的坑。

## 似乎我不会安装啊
> You must set the option boot.loader.systemd-boot.enable to true. nixos-generate-config should do this automatically for new configurations when booted in UEFI mode.
>
> You may want to look at the options starting with boot.loader.efi and boot.loader.systemd-boot as well.[^1]

好像没有设置，最后的后果即使启动一个连 ls 都没有 sh 出来

- [ ] 不要再使用完整的来作为 install 了，使用命令行可以加速调试



[^1]: https://nixos.org/manual/nixos/stable/index.html#sec-installation
[^2]: https://github.com/Martins3/Martins3.github.io/blob/master/hack/qemu/ubuntu/4-11.sh
