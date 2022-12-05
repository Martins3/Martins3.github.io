# questions

- [ ] 下面两个区别是什么?
```rs
pub mod vm;
```
vs
```rs
mod vm;
```

- [ ] 为什么要使用 use crate::addr 而不是 use addr; 如果 addr.rs 和本文件在同一级目录中
  - https://stackoverflow.com/questions/58034305/cant-import-a-file-into-another-file-in-rust

- [ ] 如何理解这个代码
```rust
let k = 10;
assert_eq!(Some(4).unwrap_or_else(|| 2 * k), 4);
assert_eq!(None.unwrap_or_else(|| 2 * k), 20);
```

- [ ] FnOnce

- [ ] 为什么需要引用这个东西
```rust
serde = "1.0.136"
serde_derive = "1.0.136"
clap = "4.0.18"
clap_derive = "4.0.18"
```
