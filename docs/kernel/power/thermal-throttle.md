# thermal-throttle

https://github.com/prometheus/node_exporter/blob/master/collector/cpu_linux.go

/sys/devices/system/cpu/cpu13/thermal_throttle
```txt
.r--r--r--  4.1k root 31 Oct 05:22   core_throttle_count
.r--r--r--  4.1k root 31 Oct 05:22   core_throttle_max_time_ms
.r--r--r--  4.1k root 31 Oct 05:22   core_throttle_total_time_ms
.r--r--r--  4.1k root 31 Oct 05:22   package_throttle_count
.r--r--r--  4.1k root 31 Oct 05:22   package_throttle_max_time_ms
.r--r--r--  4.1k root 31 Oct 05:22   package_throttle_total_time_ms
```

大致的源码在
drivers/thermal/intel/

相关的配置:
```txt
CONFIG_ACPI_THERMAL_LIB=y
CONFIG_ACPI_THERMAL=y
CONFIG_THERMAL=y
CONFIG_THERMAL_NETLINK=y
CONFIG_THERMAL_STATISTICS=y
# CONFIG_THERMAL_DEBUGFS is not set
# CONFIG_THERMAL_CORE_TESTING is not set
CONFIG_THERMAL_EMERGENCY_POWEROFF_DELAY_MS=0
CONFIG_THERMAL_HWMON=y
CONFIG_THERMAL_DEFAULT_GOV_STEP_WISE=y
# CONFIG_THERMAL_DEFAULT_GOV_FAIR_SHARE is not set
# CONFIG_THERMAL_DEFAULT_GOV_USER_SPACE is not set
# CONFIG_THERMAL_DEFAULT_GOV_BANG_BANG is not set
CONFIG_THERMAL_GOV_FAIR_SHARE=y
CONFIG_THERMAL_GOV_STEP_WISE=y
CONFIG_THERMAL_GOV_BANG_BANG=y
CONFIG_THERMAL_GOV_USER_SPACE=y
# CONFIG_THERMAL_GOV_POWER_ALLOCATOR is not set
# CONFIG_DEVFREQ_THERMAL is not set
CONFIG_PCIE_THERMAL=y
# CONFIG_THERMAL_EMULATION is not set
CONFIG_X86_THERMAL_VECTOR=y
CONFIG_X86_PKG_TEMP_THERMAL=m
# CONFIG_INTEL_SOC_DTS_THERMAL is not set
# CONFIG_INT340X_THERMAL is not set
# CONFIG_INTEL_PCH_THERMAL is not set
CONFIG_INTEL_HFI_THERMAL=y
```

## 居然就是有 thermal 的
http://www.wowotech.net/pm_subsystem/523.html

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
