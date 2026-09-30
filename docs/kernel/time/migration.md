# 太抽象了
https://lore.kernel.org/r/20240222103710.32582-1-anna-maria@linutronix.de

的确就是这么使用的:

- secondary_startup_64
  - start_secondary
    - cpu_startup_entry
      - do_idle
        - cpuidle_idle_call
          - tick_nohz_idle_stop_tick
            - tick_nohz_next_event
              - get_next_timer_interrupt
                - __get_next_timer_interrupt
                  - timer_use_tmigr
                    - tmigr_cpu_new_timer
                      - tmigr_new_timer

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
