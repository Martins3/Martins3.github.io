## qemu 的热插内存居然可以是不同的后端类型的
启动的时候，使用 memory-backend-memfd ，但是热插的时候可以是:
```txt
QEMU 10.0.94 monitor - type 'help' for more information
(qemu) object_add memory-backend-ram,id=hp_mem0,size=10G
(qemu) QEMU 10.0.94 monitor - type 'help' for more information
(qemu) device_add pc-dimm,id=hp_dimm0,memdev=hp_mem0
```

好吧，原来的确是这么操作的，需要添加一个 instance ，然后去调用 alloc 的:
```txt
2025-09-29 10:43:38 qmp_cmd_name: human-monitor-command, arguments: {"command-line": "info version"}
2025-09-29 10:43:38 qmp_cmd_name: object-add, arguments: {"qom-type": "memory-backend-ram", "size": 17179869184, "id": "memdimm0"}
[martins3:memfd_backend_instance_init:43]
[martins3:ram_backend_memory_alloc:25]
2025-09-29 10:43:38 qmp_cmd_name: device_add, arguments: {"memdev": "memdimm0", "driver": "pc-dimm", "slot": "0", "node": "0", "id": "dimm0"}
2025-09-29 10:43:38 add qdev pc-dimm:dimm0 success
2025-09-29 10:43:38 add qdev pc-dimm:dimm0 success
2025-09-29 10:43:38 qmp_cmd_name: query-memory-devices, arguments: {}
2025-09-29 10:43:38 qmp_cmd_name: qom-list, arguments: {"path": "/machine/peripheral"}
2025-09-29 10:43:38 {"timestamp": {"seconds": 1759113818, "microseconds": 501136}, "event": "ACPI_DEVICE_OST", "data": {"info": {"device": "dimm0", "source": 1, "status": 0, "slot": "0", "slot-type": "DIMM"}}}
2025-09-29 10:43:38 qmp_cmd_name: query-block, arguments: {}
2025-09-29 10:43:38 qmp_cmd_name: block_set_io_throttle, arguments: {"iops_rd": 0, "iops_wr": 0, "iops_size": 0, "bps_wr_max": 0, "iops_rd_max": 0, "bps_max": 0, "iops": 0, "bps_wr": 0, "bps_rd_max": 0, "bps": 0, "bps_rd": 0, "iops_max": 0, "id": "/machine/peripheral/virtio-disk0/virtio-backend", "iops_wr_max": 0}
```

## vhost 热插盘似乎也是建立一个新的 socket 的

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
