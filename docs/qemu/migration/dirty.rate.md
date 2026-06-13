# dirty rate
## 原来还分为这几个模式，实在是我没有想到的

```c
/* Dirty tracking enabled because migration is running */
#define GLOBAL_DIRTY_MIGRATION  (1U << 0)

/* Dirty tracking enabled because measuring dirty rate */
#define GLOBAL_DIRTY_DIRTY_RATE (1U << 1)

/* Dirty tracking enabled because dirty limit */
#define GLOBAL_DIRTY_LIMIT      (1U << 2)

#define GLOBAL_DIRTY_MASK  (0x7)
```

## 关联的源文件
- migration/dirtyrate.c
- system/dirtylimit.c

## dirty limit 的方式
有点相关的 set_vcpu_dirty_limit

## hmp 有两个对应的命令，非常类似的
- calc_dirty_rate
- info dirty_rate

## qmp shell 按照这个操作
https://wiki.qemu.org/Features/DirtyRateCalc

三种配置方法
```txt
calc-dirty-rate calc-time=3 mode=page-sampling sample-pages=1024 # 不会启动 migration thread
calc-dirty-rate calc-time=3 mode=dirty-bitmap
calc-dirty-rate calc-time=3 mode=dirty-ring
```
使用 query-dirty-rate 来查询

dirty-ring 或者 dirty-bitmap 会构建出来这两个东西:

- __clone3
  - start_thread
    - qemu_thread_start
      - get_dirtyrate_thread
        - calculate_dirtyrate
          - calculate_dirtyrate_dirty_bitmap
            - dirty_stat_wait

vcpu_dirty_rate_stat_start


如何理解这个东西?
```c
static void calculate_dirtyrate(struct DirtyRateConfig config)
{
    if (config.mode == DIRTY_RATE_MEASURE_MODE_DIRTY_BITMAP) {
        calculate_dirtyrate_dirty_bitmap(config);
    } else if (config.mode == DIRTY_RATE_MEASURE_MODE_DIRTY_RING) {
        calculate_dirtyrate_dirty_ring(config);
    } else {
        calculate_dirtyrate_sample_vm(config);
    }

    trace_dirtyrate_calculate(DirtyStat.dirty_rate);
}
```

## 额外参考
https://www.qemu.org/docs/master/devel/migration/dirty-limit.html

## 仔细看看
https://blog.csdn.net/huang987246510/article/details/133684028

## 这个东西我真的是没有想到的
https://www.qemu.org/docs/master/devel/migration/dirty-limit.html

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
