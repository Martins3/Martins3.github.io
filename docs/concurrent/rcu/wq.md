##
/home/martins3/data/vn/m/workqueue.c 中的 testcase 18

 ● 这种需求来自两个机制的叠加：释放必须等 RCU grace period，但真正的清理工作又
   不能在 RCU 回调上下文里做。

   为什么存在这种需求

   1. RCU 保护的对象不能在 reader 退出前释放，所以释放动作要推迟到 grace period
      之后。call_rcu 能解决这个问题的一半。

   2. 但 call_rcu 的回调运行在 softirq 上下文，不能睡眠、不能拿 mutex、不能做重
      活。而很多清理工作恰恰需要进程上下文，比如：
       • kmem_cache_destroy() 要拿锁、要 shrink slab
       • 释放嵌套的大对象、关闭文件、操作 sysfs/kobject
       • 清理本身还要 flush 其他 workqueue

   3. 于是就拼出了 queue_rcu_work：call_rcu 等 grace period → 回调里 queue_work
       把真正的清理扔到进程上下文执行。

   4. 有了这种"双重延后"的执行，拆除路径（模块卸载、对象销毁）就必须能等它彻底
      跑完，否则 work 访问的内存可能已经被释放了。普通的 flush_work 不够——grace
      period 没过时 work 根本还没入队，flush_work 会立即返回，造成假象。这就是
      flush_rcu_work 存在的理由。

   另外它也比 synchronize_rcu() + 直接清理 更灵活：synchronize_rcu 会阻塞调用者
   一整个 grace period，而 queue_rcu_work 是纯异步的，调用者（可能是 atomic 上
   下文）拍完屁股就走。

   经典使用场景（内核里的真实例子）

   • fs/aio.c:692 —— rcu_work 的"开山"场景。kioctx 的 table->table[] 查找是 RCU
     保护的，free_ioctx 要做大量可睡眠的清理，所以
     INIT_RCU_WORK(&ctx->free_rwork, free_ioctx); queue_rcu_work(...)。

   • mm/slab_common.c —— kmem_cache 销毁：kmem_cache_release 用 rcu_work 在
     grace period 之后才真正拆 cache（要拿 slab 锁、offload per-cpu sheaf 等
     ）；同文件的 kvfree_rcu 批量回收路径里，per-cpu 的 flush_rcu_work() 用来等
     之前 queue 的批量释放 work 跑完再挂新批次。

   • kernel/cgroup/cgroup.c —— css（cgroup subsystem state）的释放
     css_free_rwork_fn，cgroup 生命周期结束、reader 退出后才释放子系统状态。

   • 网络子系统（net/sched/cls_api.c、drivers/net/macsec.c、
     net/netfilter/nfnetlink_queue.c 等）—— RCU 保护的转发表/规则对象，删除后要
     等 grace period，再在进程上下文做解引用计数、释放内存等复杂收尾。

   一句话总结：call_rcu 管"什么时候安全"，workqueue 管"在什么上下文干活
   "，flush_rcu_work 管"拆除时怎么确认活干完了"。三者合起来覆盖了"RCU 保护 + 可
   睡眠清理 + 安全拆除"这个完整闭环。



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
