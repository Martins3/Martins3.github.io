# trace 机制实现
<!-- c8c5dfb8-df2e-48c4-9ef6-cf793c1dba30 -->

随便想象，如果我来实现一个 trace 机制，需要考虑什么问题:

## 解析 stack
这绝对是一个复杂的事情
	-  https://news.ycombinator.com/item?id=35592446

## 处理符号信息

## 未曾想到的复杂
- CONFIG_TASKS_RCU rcu 需要特殊考虑 ftrace 等插桩中的资源释放问题
	- https://docs.kernel.org/RCU/Design/Requirements/Requirements.html#tasks-rcu

## gcc 和 llvm

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
