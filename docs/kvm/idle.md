## 先搞搞最基本的问题吧

- 如果不 idle，是不是可以完全没有 kvm_exit，还是会因为 host 的 timer 退出?

- 时钟中断可以直接注入到 guest 中间吗? 还是需要让 guest 推出一下

  - 我猜测这个是可以控制的
  - idle 的时候退出我猜测是因为 halt 指令模拟
  - 测试下，当如果打开 guest NO_HZ 和关闭 NO_HZ 的时候，exit 的差别

- 本来 guest 因为 halt 退出到 host，如果 guest 忽然变忙了，host 如何知道?

  - host 必然会接受到应该注入到 guest 的中断。
  - 只要 kernel_args+=" nohz_full=0-7" 会导致 8 core 的 exit 为 2000/s，去掉之后，exit 的数量 200/s ，有趣啊

- 为什么 kvm_task_switch 从来不会有人调用
  - 我还以为只要 kvm 出现 process switch 的时候，就会发生调用的啊

./features/pv-sched-yield.md
./docs/kvm/halt-polling.md : 其实如果可以不退出，那不就是不需要 halt-polling 吗?
./preemption_timer.md

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
