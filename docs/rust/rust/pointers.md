# rust 的 smart pointers
<!-- d35e0a6f-09d4-4cf9-a38b-9e1eaf8402de -->

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

## 然后继续理解下，为什么 loop 的会导致 memory leak

strong 和 weak 的作用就可以了

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
