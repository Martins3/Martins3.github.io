## acpi_power_meter
<!-- 7f0fd321-3968-4045-b9c1-9964d234cd5d -->

- https://www.suse.com/support/kb/doc/?id=000017865
- https://access.redhat.com/solutions/49225?_refluxos=a10
- https://askubuntu.com/questions/786558/power-meter-acpi000d00-ignoring-unsafe-software-power-cap
- https://learn.microsoft.com/zh-cn/windows-hardware/drivers/powermeter/

从用户态读取:
https://github.com/prometheus/node_exporter

```txt
$ find /sys/devices/LNXSYSTM\:00/ |grep ACPI000D
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/uevent
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/power1_average_interval
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/power1_accuracy
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/power
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/power/runtime_active_time
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/power/runtime_status
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/power/autosuspend_delay_ms
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/power/runtime_suspended_time
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/power/control
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/physical_node
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/power1_serial_number
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/power1_average_interval_min
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/hid
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/power1_oem_info
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/power1_average
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/driver
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/power1_model_number
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/path
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/power1_is_battery
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/power1_average_interval_max
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/subsystem
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/status
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/name
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/measures
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/measures/LNXSYBUS:00
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/hwmon
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/hwmon/hwmon1
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/hwmon/hwmon1/uevent
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/hwmon/hwmon1/power
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/hwmon/hwmon1/power/runtime_active_time
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/hwmon/hwmon1/power/runtime_status
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/hwmon/hwmon1/power/autosuspend_delay_ms
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/hwmon/hwmon1/power/runtime_suspended_time
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/hwmon/hwmon1/power/control
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/hwmon/hwmon1/device
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/hwmon/hwmon1/subsystem
/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/modalias
```

触发的 backtrace :
```txt
[root@dogfood-idc-elf-19-21-NVME-M2 15:42:43 tools]$ ./stackcount acpi_ev_address_space_dispatch
  acpi_ev_address_space_dispatch
  acpi_ex_access_region
  acpi_ex_write_data_to_field
  acpi_ex_store_object_to_node
  acpi_ex_store
  acpi_ex_opcode_1A_1T_1R
  acpi_ds_exec_end_op
  acpi_ps_parse_loop
  acpi_ps_parse_aml
  acpi_ps_execute_method
  acpi_ns_evaluate
  acpi_evaluate_object
  acpi_evaluate_integer
  update_meter
  show_power
  dev_attr_show
  sysfs_kf_seq_show
  seq_read
  __vfs_read
  vfs_read
  ksys_read
  do_syscall_64
  entry_SYSCALL_64_after_hwframe
```

需要加载模块:
```txt
config ACPI_IPMI
	tristate "IPMI"
	depends on IPMI_HANDLER
	default n
	help
	  This driver enables the ACPI to access the BMC controller. And it
	  uses the IPMI request/response message to communicate with BMC
	  controller, which can be found on on the server.

	  To compile this driver as a module, choose M here:
	  the module will be called as acpi_ipmi.
```

将 acpi_ipmi 去掉，不然立刻出现:
```txt
[1459302.075158] ACPI Error: No handler for Region [POWR] (0000000048e21840) [IPMI] (20180810/evregion-132)
[1459302.086825] ACPI Error: Region IPMI (ID=7) has no handler (20180810/exfldio-265)
[1459302.097239] ACPI Error: Method parse/execution failed \_SB.PMI0._PMM, AE_NOT_EXIST (20180810/psparse-516)
[1459302.109812] ACPI Error: AE_NOT_EXIST, Evaluating _PMM (20180810/power_meter-338)
[1459332.054620] ACPI Error: No handler for Region [POWR] (0000000048e21840) [IPMI] (20180810/evregion-132)
[1459332.066593] ACPI Error: Region IPMI (ID=7) has no handler (20180810/exfldio-265)
[1459332.078556] ACPI Error: Method parse/execution failed \_SB.PMI0._PMM, AE_NOT_EXIST (20180810/psparse-516)
[1459332.091514] ACPI Error: AE_NOT_EXIST, Evaluating _PMM (20180810/power_meter-338)
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
