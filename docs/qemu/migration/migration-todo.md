# 暂时没有时间看这个问题，但是之后有时间过来看

## 时钟在热迁移的时候需要如何处理？
```c
    if (!env->user_tsc_khz) {
        if ((env->features[FEAT_8000_0007_EDX] & CPUID_APM_INVTSC) &&
            invtsc_mig_blocker == NULL) {
            error_setg(&invtsc_mig_blocker,
                       "State blocked by non-migratable CPU device"
                       " (invtsc flag)");
            r = migrate_add_blocker(invtsc_mig_blocker, &local_err);
            if (r < 0) {
                error_report_err(local_err);
                return r;
            }
        }
    }
```
