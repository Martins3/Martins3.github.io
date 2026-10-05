# neovide

[neovide](https://github.com/Kethku/neovide) neovim 客户端，只有 7000 行

这个程序是一个图形程序，所以一定是需要支持 wayland 的

例如发可以从中找到: src/window/window_wrapper.rs

学习下他的 profile 技术:

cargo run --profile profiling --features profiling -- --no-vsync --no-multigrid
https://github.com/neovide/neovide/issues/2602
