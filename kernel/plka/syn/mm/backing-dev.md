# backing-dev.c

## KeyNote
1. 当其中的 cgroup 被清理掉之后，代码的行数将会减少一般


## init

```c
static int bdi_init(struct backing_dev_info *bdi) // 初始化 bdi
    static int cgwb_bdi_init(struct backing_dev_info *bdi) // malloc wb
        static int wb_init(struct bdi_writeback *wb, struct backing_dev_info *bdi, int blkcg_id, gfp_t gfp) // 初始化 wb
```

