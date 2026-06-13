## 写 rust 的工具

试试
cargo clean
- miri

不知道为什么会有错误:
```txt
🧀  rustup component add miri
error: component 'miri' for target 'x86_64-unknown-linux-gnu' is unavailable for download for channel '1.85'
```

nix 中不知道为什么 rust-analyzer 是指向了软连接的:
```txt
l ~/.cargo/bin/rust-analyzer
Permissions Size User     Date Modified Name
lrwxrwxrwx     - martins3 28 Feb 17:22  󰡯 /home/martins3/.cargo/bin/rust-analyzer -> rustup
```


## rust 继续

先搞这个 "mrcjkb/rustaceanvim",

## 用 rust 写的工具
- https://github.com/feschber/lan-mouse : 局域网共享键鼠
- Raptor: Realtime Abstracted Path Tree Observer
	- https://github.com/ErickJ3/raptor
- https://github.com/sharkdp/hexyl : 显示文件的二进制，其实，我只是奇怪，为什么这么简单的程序会有 5k star
- https://github.com/uutils/coreutils


## 其实 nix 环境中，升级 rust 也是很简单的

每次就是执行这些命令就可以了:
```sh
# 使用 mirrors 可以极大的加速，在非配置环境中可以使用
# export RUSTUP_DIST_SERVER=https://mirrors.tuna.tsinghua.edu.cn/rustup
rustup update
rustup default stable && rustc --version
rustup component add rust-analyzer
```

```txt

info: syncing channel updates for 'stable-x86_64-unknown-linux-gnu'
info: syncing channel updates for 'nightly-x86_64-unknown-linux-gnu'
info: syncing channel updates for '1.65-x86_64-unknown-linux-gnu'
info: syncing channel updates for '1.85-x86_64-unknown-linux-gnu'
info: syncing channel updates for '1.89-x86_64-unknown-linux-gnu'
info: checking for self-update

   stable-x86_64-unknown-linux-gnu unchanged - rustc 1.94.0 (4a4ef493e 2026-03-02)
  nightly-x86_64-unknown-linux-gnu unchanged - rustc 1.96.0-nightly (b41f22de2 2026-03-08)
     1.65-x86_64-unknown-linux-gnu unchanged - rustc 1.65.0 (897e37553 2022-11-02)
     1.85-x86_64-unknown-linux-gnu unchanged - rustc 1.85.1 (4eb161250 2025-03-15)
     1.89-x86_64-unknown-linux-gnu unchanged - rustc 1.89.0 (29483883e 2025-08-04)

info: cleaning up downloads & tmp directories

info: using existing install for 'stable-x86_64-unknown-linux-gnu'
info: default toolchain set to 'stable-x86_64-unknown-linux-gnu'

  stable-x86_64-unknown-linux-gnu unchanged - rustc 1.94.0 (4a4ef493e 2026-03-02)

rustc 1.94.0 (4a4ef493e 2026-03-02)
```

## 很好
https://github.com/emilk/egui

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
