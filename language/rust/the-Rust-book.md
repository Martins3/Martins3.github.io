### 10.3
为什么 Rust 需要 lifetime annotation : 为了描述 references 的声明周期, 书中的例子，比较两个字符串，返回 references 的时候，两者生命周期不同，导致返回值的使用返回不同，所以需要标注

函数声明 lifetime elision 的原则[^1], 为了解决函数返回值生命周期
1. 没有返回值，可以省略
2. 有 `&self` `&mut self` 可以省略，因为按照 self
3. 参数的 lifetime 都是一致的


对于第二条，问题是，如果 self 的声明周期 和 其他参数不一致会怎样 ?
代码的逻辑的逻辑会被检查
```rs
struct Name<'a> {
    x: &'a str,
    y: &'a str,
}

impl<'a> Name<'a> {
    fn longest(&self, y: &str) -> &str {
        if self.x.len() > y.len() {
            self.x
        } else {
            // 将
            self.y
        }
    }
}
```

## 15.5
- [ ] TODO 这一章应该算是终结了

这个回答是一个不错的总结:
https://stackoverflow.com/questions/45674479/need-holistic-explanation-about-rusts-cell-and-reference-counted-types
  - 更加丰富的总结 : ![https://github.com/usagi/rust-memory-container-cs](https://media.githubusercontent.com/media/usagi/rust-memory-container-cs/master/3840x2160/rust-memory-container-cs-3840x2160-dark-back-low-contrast.png)

- [x] 所以单线程中间会出现 Cell 的动态检查不通过的情况吗 ?
  - Rust Book 给出的例子是 : 当连续调用两次 borrow_mut，那么就会出现问题, 必须等到第一个 borrow_mut 的生命周期结束才可以。
  - [ ] 但是我感觉这种操作，为什么需要动态的检查

A common way to use `RefCell<T>` is in combination with `Rc<T>`
一个可以存在多个 owner 同时修改了。

- [ ] 最后使用了 listed list 的例子, 但是无法理解没有了 RefCell 会出现什么问题。

[^1]: https://learning-rust.github.io/docs/c3.lifetimes.html

