# Extended Attributes

Extended Attribute
allow arbitrary metadata,
in the form of name-value pairs, to be associated with file i-nodes.

## 16.1 Overview
EAs are used to implement access control lists (Chapter 17) and file capabilities (Chapter 39).

> 文件 icon 的实现基础，awesome

An i-node may have multiple associated EAs, in the same namespace or in different
namespaces. The EA names within each namespace are distinct sets. In the user and
trusted namespaces, EA names can be arbitrary strings. In the system namespace,
only names explicitly permitted by the kernel

> 演示了 setfattr 和 getfattr 的最基本的使用，但是 Ubuntu 上可以运行，但是 Manjaro 无法运行

## 16.2 Extended Attribute Implementation Details
> 介绍了 C 语言的调用的函数，最后还提供了一个编程例子

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
