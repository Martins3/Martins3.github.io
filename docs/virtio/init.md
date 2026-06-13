## 设备的初始化
- virtio_device::private 指向具体的 virtio 设备
- virtio_blk / virtio_balloon 具体的设备只有 virtio_device 指针

- virtio_pci_device 包含 virtio_device

系统启动的过程中，一共会 probe 两次:

- ret_from_fork
  - kernel_init
    - kernel_init_freeable
      - do_basic_setup
        - do_initcalls
          - do_initcall_level
            - do_one_initcall
              - driver_register
                - bus_add_driver
                  - driver_attach
                    - bus_for_each_dev
                      - __driver_attach
                        - __driver_attach
                          - driver_probe_device
                            - __driver_probe_device
                              - really_probe
                                - call_driver_probe
                                  - pci_device_probe
                                    - __pci_device_probe
                                      - pci_call_probe
                                        - local_pci_probe
                                          - virtio_pci_probe
                                            - virtio_pci_modern_probe

  - ret_from_fork
    - kernel_init
      - kernel_init_freeable
        - do_basic_setup
          - do_initcalls
            - do_initcall_level
              - do_one_initcall
                - driver_register
                  - bus_add_driver
                    - driver_attach
                      - bus_for_each_dev
                        - __driver_attach
                          - __driver_attach
                            - driver_probe_device
                              - __driver_probe_device
                                - really_probe
                                  - call_driver_probe
                                    - virtio_dev_probe
                                      - virtballoon_probe

这个参考更加清晰: ![](https://img2020.cnblogs.com/blog/1771657/202102/1771657-20210224230417971-437424297.png)

```c
static struct pci_driver virtio_pci_driver = {
  .name   = "virtio-pci",
  .id_table = virtio_pci_id_table,
  .probe    = virtio_pci_probe,
  .remove   = virtio_pci_remove,
#ifdef CONFIG_PM_SLEEP
  .driver.pm  = &virtio_pci_pm_ops,
#endif
  .sriov_configure = virtio_pci_sriov_configure,
};
```

```c
static struct bus_type virtio_bus = {
  .name  = "virtio",
  .match = virtio_dev_match,
  .dev_groups = virtio_dev_groups,
  .uevent = virtio_uevent,
  .probe = virtio_dev_probe,
  .remove = virtio_dev_remove,
};
```

```c
static struct virtio_driver virtio_blk = {
  .feature_table      = features,
  .feature_table_size   = ARRAY_SIZE(features),
  .feature_table_legacy   = features_legacy,
  .feature_table_size_legacy  = ARRAY_SIZE(features_legacy),
  .driver.name      = KBUILD_MODNAME,
  .driver.owner     = THIS_MODULE,
  .id_table     = id_table,
  .probe        = virtblk_probe,
  .remove       = virtblk_remove,
  .config_changed     = virtblk_config_changed,
#ifdef CONFIG_PM_SLEEP
  .freeze       = virtblk_freeze,
  .restore      = virtblk_restore,
#endif
};
```

这个架构很有趣，同时挂载在 pci 和 virtio 两个总线上。

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
