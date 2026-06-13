# rdp

## 对于远程的一切都好奇
- https://github.com/miroslavpejic85/p2p

- https://rustdesk.com/zh/ (这个用的是 windows 原生的吗?)
- https://github.com/screego/server
- https://github.com/kunkundi/crossdesk

## IronRDP
- https://github.com/Devolutions/IronRDP
	- https://github.com/winapps-org/winapps 用的是 https://github.com/FreeRDP

2026-06-07

使用 dotfiles 中的 scripts/nix/env/ironrdp.nix ，cargo build ，但是运行有点问题
以后再看吧:
```txt
./target/debug/ironrdp-viewer 192.168.1.100 --username admin  --password secret

Error: os error at /home/martins3/.cargo/registry/src/index.crates.io-1949cf8c6b5b557f/winit-0.30.13/src/platform_impl/linux/mod.rs:765: neither WAYLAND_DISPLAY nor WAYLAND_SOCKET nor DISPLAY is set.
```

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
