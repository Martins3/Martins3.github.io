## swait

很少的 user 使用 swait
```txt
- sysvec_apic_timer_interrupt
  - __irq_exit_rcu
    - invoke_softirq
      - __do_softirq
        - rcu_core
          - rcu_check_quiescent_state
            - rcu_report_qs_rdp
              - swake_up_one
                - swake_up_locked
                  - swake_up_locked
                    - wake_up_process
                      - try_to_wake_up
                        - set_task_cpu
                          - migrate_task_rq_fair
```

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
