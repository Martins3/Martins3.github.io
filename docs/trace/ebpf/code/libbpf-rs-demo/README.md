# libbpf-rs demo
<!-- 6069b98b-fbfd-420d-85fa-62981168874a -->

这是一个最小的 Rust eBPF 示例，使用 [libbpf-rs](https://github.com/libbpf/libbpf-rs)
和 `libbpf-cargo`：

- `build.rs` 编译 `src/bpf/trace.bpf.c`，并自动生成 Rust skeleton；
- BPF 程序挂载到 `sys_enter_write` tracepoint；
- 用户态程序通过 `RingBufferBuilder` 接收事件并打印进程、文件描述符和写入字节数。

libbpf-rs 并不是“用 Rust 编写 eBPF 程序”的框架。若希望 BPF 程序本身也用 Rust，可以看 Aya；它的用户态和 eBPF 侧都能使用 Rust，
但工具链和生态路线与 libbpf/CO-RE 不同。

## 基本操作
```sh
cargo build
sudo ./target/debug/libbpf-rs-demo
sudo ./target/debug/libbpf-rs-demo --pid <PID>
```

## build.rs 是 Cargo 的构建脚本，在编译 Rust 用户态程序之前执行。这里它主要做两件事：

1. SkeletonBuilder::build_and_generate() 使用 Clang 把 trace.bpf.c 编译成 eBPF ELF。
2. 根据该 ELF 生成 trace.skel.rs，提供类型安全的 Rust 接口。

随后 src/main.rs 通过：

```rust
include!(concat!(
    env!("CARGO_MANIFEST_DIR"),
    "/src/bpf/trace.skel.rs"
));
```

引入生成的 skeleton，才能使用 TraceSkelBuilder、skel.maps.events 等接口。

build.rs 里的 cargo:rerun-if-changed 还会通知 Cargo：修改 BPF C 文件或共享头文件后，必须重新生成 skeleton。

没有 build.rs 也能实现，但每次修改 BPF 代码后都得手动执行类似：

```txt
cargo libbpf build
cargo libbpf gen
cargo build
```

所以它的核心作用是把“编译 BPF C -> 生成 Rust skeleton -> 编译 Rust loader”串成一次普通的 cargo build。

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
