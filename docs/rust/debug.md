# rust gdb 基本使用方法
<!-- fcf7bb04-a0b0-4d14-bf01-518a1183a5a0 -->

```txt
  cd /home/martins3/data/vn/docs/rust/demo
  cargo build
  rust-gdb target/debug/demo
```

进入 GDB 后：

```txt
  set language rust
  rbreak linked_list.*drop
  run 14 # 对应 cargo run -- 14
```

## 调试测试程序
cargo test --no-run

这个命令会输出测试程序路径，例如：

target/debug/deps/demo-9096a9d14d5aaf3e

把实际路径传给 rust-gdb：

rust-gdb target/debug/deps/demo-9096a9d14d5aaf3e

然后执行：

set language rust
rbreak linked_list.*drop
run linked_list::test::iter --exact --nocapture

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
