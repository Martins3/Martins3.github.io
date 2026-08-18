# GPU scheduler
<!-- 5c30b6fc-46fd-47f6-b4ca-25d539c414ef -->

可以 trace 一下这些内容:
- gpu_scheduler:drm_sched_job_queue
- gpu_scheduler:drm_sched_job_run
- gpu_scheduler:drm_sched_job_add_dep
- gpu_scheduler:drm_sched_job_done
- gpu_scheduler:drm_sched_job_unschedulable

https://zhuanlan.zhihu.com/p/641331417

调查下，似乎这个的确和 cuda green context ，
不过我猜测，GPU 调度的核心是在 GPU 内部吧。


## 看看 nvidia 的配置
nvidia-smi -q | grep "Compute Mode"

## 只有 xe 才支持?
```txt
lsmod | grep gpu_sched
gpu_sched              73728  1 xe
```
