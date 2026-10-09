# 智能指针：从“谁来 delete”到链表的所有权

智能指针的动机是：**把释放资源的责任写进对象和类型，让正常返回、提前返回和异常退出都沿用同一套清理规则。**

普通指针保存地址，但从 `Node* p` 本身看不出：这是借来的地址，还是需要你负责释放的对象？
智能指针把“能访问”与“负责生命周期”区分开。

按下面的顺序阅读，示例采用 C++17：

1. [动机与三种智能指针](motivation.md)：手动管理哪里容易出错，所有权为什么要进入类型。
2. [用 unique_ptr 管理链表](linked-list.md)：拥有关系、插入、删除、遍历以及整条链的清理。
3. [共享所有权与循环引用](shared-ownership.md)：shared_ptr 为什么不能自动解决所有图结构，weak_ptr 的用途。

运行全部示例：

```sh
cd /home/martins3/data/vn/docs/cpp/smart-pointer
make run
```

| 示例 | 重点观察 |
| --- | --- |
| [ownership.cpp](ownership.cpp) 的 `work()` | 所有权移动后地址不变；抛异常后仍先析构，再进入 catch |
| [unique-list.cpp](unique-list.cpp) 的 `List` | 节点删除、所有权交接和链表析构 |
| [shared-weak.cpp](shared-weak.cpp) 的 `main()` | 强引用成环会留住对象，弱引用回指不留住对象 |

## 正常来说，是不需要 new 和 delete 的

使用 zero rule 就可以了

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
