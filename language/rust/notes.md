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
