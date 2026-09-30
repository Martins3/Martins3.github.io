## qemu patch 代码
https://docs.google.com/document/d/1aO2fQrF-IwcRyB5FfP1j2Iy8HhKhDD89lcLnXOglWBY/edit?tab=t.0


所以 local migration 出现时间跳变显然是有问题的啊!

热迁移的时候会把相同的 tsc 写回去，那么我们有保证写入的都是不同的。

虚拟机中报错为:
```txt
[   29.743439] clocksource: timekeeping watchdog on CPU26: Marking clocksource 'tsc' as unstable because the skew is too large:
[   29.744480] clocksource:                       'kvm-clock' wd_nsec: 496006007 wd_now: 6fae14200 wd_last: 6dd50ce89 mask: ffffffffffffffff
[   29.745536] clocksource:                       'tsc' cs_nsec: 480609826 cs_now: 14eb8565d9 cs_last: 1495b8075c mask: ffffffffffffffff
[   29.746143] clocksource:                       Clocksource 'tsc' skewed -15396181 ns (-15 ms) over watchdog 'kvm-clock' interval of 496006007 ns (496 ms)
[   29.746770] clocksource:                       'kvm-clock' (not 'tsc') is current clocksource.
[   29.747130] tsc: Marking TSC unstable due to clocksource watchdog
```

添加这个代码，可以观察到这些效果:
```diff
diff --git a/arch/x86/kvm/x86.c b/arch/x86/kvm/x86.c
index 01d3fa84d2a4..7e090ac01fa1 100644
--- a/arch/x86/kvm/x86.c
+++ b/arch/x86/kvm/x86.c
@@ -2738,6 +2738,7 @@ static void kvm_synchronize_tsc(struct kvm_vcpu *vcpu, u64 *user_value)
 		matched = true;
 	}

+	pr_info("[martins3:%s:%d] %lld\n", __FUNCTION__, __LINE__, offset);
 	__kvm_synchronize_tsc(vcpu, offset, data, ns, matched);
 	raw_spin_unlock_irqrestore(&kvm->arch.tsc_write_lock, flags);
 }
@@ -3905,6 +3906,7 @@ int kvm_set_msr_common(struct kvm_vcpu *vcpu, struct msr_data *msr_info)
 		break;
 	case MSR_IA32_TSC:
 		if (msr_info->host_initiated) {
+			pr_info("[martins3:%s:%d] %d %lld\n", __FUNCTION__, __LINE__, vcpu->vcpu_id, data);
 			kvm_synchronize_tsc(vcpu, &data);
 		} else {
 			u64 adj = kvm_compute_l1_tsc_offset(vcpu, data) - vcpu->arch.l1_tsc_offset;
```

为什么，每一个 vCPU 的 l1 都是不同的，因为 __get_kvmclock 在调用的时候获取到的 rdtsc ，所以发现
从 source 端写如到 l1 tsc offset 的时候数值各部相同，但是最后写入到 l1 tsc offset 的时候，他们都是相同的
```txt
[31571.626913] kvm: [martins3:kvm_set_msr_common:3909] 0 61377011034
[31571.626917] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.626963] kvm: [martins3:kvm_set_msr_common:3909] 1 61378056818
[31571.626966] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.626990] kvm: [martins3:kvm_set_msr_common:3909] 2 61378393037
[31571.626991] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627007] kvm: [martins3:kvm_set_msr_common:3909] 3 61378558580
[31571.627008] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627045] kvm: [martins3:kvm_set_msr_common:3909] 4 61378762498
[31571.627048] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627076] kvm: [martins3:kvm_set_msr_common:3909] 5 61378899217
[31571.627078] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627106] kvm: [martins3:kvm_set_msr_common:3909] 6 61379117945
[31571.627108] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627187] kvm: [martins3:kvm_set_msr_common:3909] 7 61379263214
[31571.627190] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627226] kvm: [martins3:kvm_set_msr_common:3909] 8 61379397999
[31571.627228] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627321] kvm: [martins3:kvm_set_msr_common:3909] 9 61379419622
[31571.627324] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627399] kvm: [martins3:kvm_set_msr_common:3909] 10 61379438312
[31571.627402] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627426] kvm: [martins3:kvm_set_msr_common:3909] 11 61379455334
[31571.627427] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627448] kvm: [martins3:kvm_set_msr_common:3909] 12 61379630869
[31571.627450] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627475] kvm: [martins3:kvm_set_msr_common:3909] 13 61379777486
[31571.627476] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627545] kvm: [martins3:kvm_set_msr_common:3909] 14 61379807984
[31571.627548] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627575] kvm: [martins3:kvm_set_msr_common:3909] 15 61379823778
[31571.627576] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627601] kvm: [martins3:kvm_set_msr_common:3909] 16 61379842062
[31571.627602] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627621] kvm: [martins3:kvm_set_msr_common:3909] 17 61380105225
[31571.627621] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627642] kvm: [martins3:kvm_set_msr_common:3909] 18 61380124582
[31571.627644] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627677] kvm: [martins3:kvm_set_msr_common:3909] 19 61380259413
[31571.627679] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627711] kvm: [martins3:kvm_set_msr_common:3909] 20 61380440235
[31571.627712] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627728] kvm: [martins3:kvm_set_msr_common:3909] 21 61380670969
[31571.627729] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627750] kvm: [martins3:kvm_set_msr_common:3909] 22 61380802501
[31571.627752] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627777] kvm: [martins3:kvm_set_msr_common:3909] 23 61380830293
[31571.627779] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627804] kvm: [martins3:kvm_set_msr_common:3909] 24 61381027467
[31571.627805] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627828] kvm: [martins3:kvm_set_msr_common:3909] 25 61381161637
[31571.627829] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627857] kvm: [martins3:kvm_set_msr_common:3909] 26 61381427823
[31571.627858] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627887] kvm: [martins3:kvm_set_msr_common:3909] 27 61381553779
[31571.627888] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627912] kvm: [martins3:kvm_set_msr_common:3909] 28 61381695495
[31571.627913] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627938] kvm: [martins3:kvm_set_msr_common:3909] 29 61381847440
[31571.627940] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627969] kvm: [martins3:kvm_set_msr_common:3909] 30 61381862988
[31571.627971] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
[31571.627995] kvm: [martins3:kvm_set_msr_common:3909] 31 61382085053
[31571.627997] kvm: [martins3:kvm_synchronize_tsc:2741] -94577488205333
```

## 既然虚拟机不变，为什么 kvm-clock 会变化，其实说不清楚谁错了。。。

还是说，tsc 的时间发生了回退？

## master clock 的切换了吗？

## 比对 guest 中计算的 pvti 和我们配置的 pvti 的结果


## 4.19 热迁移问题 : master clock 是 mono 的话，会有问题吗?

并不会。只是

## 为什么 firecracker 可以让 tsc 默认为

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
