## Ownership and lifetime
<!-- 9621368f-b648-41e2-ba4a-04a08a94a654 -->

[这个](https://www.reddit.com/r/rust/comments/mgh9n9/ownership_concept_diagram/)
对于 ownership 的总结，实际上，技术就是 Rc 和 Cell

https://github.com/usagi/rust-memory-container-cs/blob/master/3840x2160/rust-memory-container-cs-3840x2160-dark-back.png

所以，Cell 的到底是怎么实现的呀?
- [ ] Cell 增加了代码进行检查，那么到底检查的内容是什么 ?

看看这个教程了:
https://github.com/tfpk/lifetimekata

borrow -> cell
clone -> rc

## 检查工具

https://github.com/cordx56/rustowl


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
