## Rust Atomics and Locks
https://marabos.nl/atomics/basics.html

## rpc
简单读了下，这是绝对应该阅读的东西

```sh
./target/debug/helloworld-server
./target/debug/helloworld-client
grpcurl -plaintext -import-path ./proto -proto helloworld.proto -d '{"name": "Tonic"}' '[::1]:50051' helloworld.Greeter/SayHello
```

## pingora
```sh
RUST_LOG=INFO ./target/debug/pg  -c ./src/pingora/conf.toml  -d
```

## 测试
cargo test rustbook

覆盖率
cargo install cargo-tarpaulin
cargo tarpaulin --out Html

## 基本遇到放到 demo 中测试

使用方法
```sh
cargo build --package demo && target/debug/demo 2
```

## rust async

- 也许，我们应该增加一个项目，叫做不同的语言是如何设计锁的:
  - https://course.rs/advance/concurrency-with-threads/thread.html

基于 Rust 的
https://news.ycombinator.com/item?id=34271739

https://github.com/PacktPublishing/Asynchronous-Programming-in-Rust/blob/main/ch02/a-os-threads/src/main.rs

## rust 中如何设计锁的
- https://github.com/m-ou-se/rust-atomics-and-locks
- https://github.com/rustcc/Rust_Atomics_and_Locks


https://www.scylladb.com/2022/01/12/async-rust-in-practice-performance-pitfalls-profiling/
https://eta.st/2021/03/08/async-rust-2.html

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
