# QEMU 如何加载 Linux kernel image
QEMU 提供了 -kernel 参数，让 guest 运行的内核可以随意指定，这对于调试内核非常的方便，现在说明一下 -kernel 选项是如何实现的:

阅读本文需要大致了解 [fw_cfg](./fw_cfg.md) 的知识

## QEMU's preparation
1. 通过 [QEMU 的参数解析](https://martins3.github.io/qemu/options.html) 机制，将参数保存到 MachineState::kernel_filename 中
```c
static void machine_set_kernel(Object *obj, const char *value, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    g_free(ms->kernel_filename);
    ms->kernel_filename = g_strdup(value);
}
```

2. 在 `x86_load_linux` 中添加 linuxboot_dma.bin 到 `option_rom` 数组中

```c
    f = fopen(kernel_filename, "rb");

    if (fread(kernel, 1, kernel_size, f) != kernel_size) { // 读去文件内容
        fprintf(stderr, "fread() failed\n");
        exit(1);
    }

    fw_cfg_add_bytes(fw_cfg, FW_CFG_KERNEL_DATA, kernel, kernel_size); // 通过 FW_CFG_KERNEL_DATA 告知 seabios

    option_rom[nb_option_roms].bootindex = 0;
    option_rom[nb_option_roms].name = "linuxboot.bin";
    if (linuxboot_dma_enabled && fw_cfg_dma_enabled(fw_cfg)) {
        option_rom[nb_option_roms].name = "linuxboot_dma.bin";
    }
```

3. 在 pc_memory_init 中调用 rom_add_option 添加到 fw_cfg 中，之后 seabios 就可以通过 fw_cfg 读取 `linuxboot_dma.bin`
```c
    for (i = 0; i < nb_option_roms; i++) {
        rom_add_option(option_rom[i].name, option_rom[i].bootindex);
    }
```

4. rom_add_option 会进一步调用 add_boot_device_path 中，记录到 `fw_boot_order`，从而让 get_boot_devices_list 可以返回 "/rom@genroms/linuxboot_dma.bin"

5. fw_cfg_machine_reset 中修改 "bootorder"
```c
    buf = get_boot_devices_list(&len); // 返回内容 /rom@genroms/linuxboot_dma.bin
    ptr = fw_cfg_modify_file(s, "bootorder", (uint8_t *)buf, len);
```

到此，QEMU 的准备完成，实际上就是修改 "bootorder"，让 seabios 通过执行 linuxboot_dma.bin 来启动

## Seabios 的基本执行流程

- maininit
  - interface_init
    - boot_init
      - loadBootOrder : 构建 Bootorder
  - optionrom_setup
    - run_file_roms
      - deploy_romfile : **将 linuxboot_dma.bin 加载进来**
      - init_optionrom
        - callrom : 执行 linuxboot_dma.bin 部分代码，初始化 pnp 相关内容
      - setRomSource
    - get_pnp_rom : linuxboot_dma.bin 是按照 pnp 规则的构建的 optionrom
    - boot_add_bev : Registering bootable: Linux loader DMA (type:128 prio:1 data:cb000054)
      - getRomPriority
        - find_prio : 根据 Bootorder 的内容返回 prio
      - bootentry_add : 将 kernel image 添加到 BootList 中，在 BootList 的排序根据 getRomPriority 获取的 prio 确定
  - prepareboot
    - bcv_prepboot : 连续调用 add_bev, 调用顺序是按照 BootList 构建 `BEV`
  - startBoot
    - call16_int(0x19, &br)
      - handle_19
        - do_boot
          - boot_rom : 默认使用第一个 BEV，也就是 kernel image
            - call_boot_entry : linuxboot_dma.bin 上，然后 linuxboot_dma.bin 进一步跳转到 kernel image 上开始执行

其实，总体来说，seabios 做了两个事情:
- 执行 optionrom linuxboot_dma.bin 将 linuxboot_dma.bin 注册到 BootList 中
- 根据 "bootorder" 将 linuxboot_dma.bin 作为优先级最高的启动方式
- 执行 linuxboot_dma.bin 的第二部分，在其中通过 fw_cfg 获取 kernel 的入口地址、参数地址，并且通过 *fw_cfg* 加载内核过来。

## linuxboot_dma.bin 源代码解析

linuxboot_dma.bin 是通过 `pc-bios/optionrom/linuxboot_dma.c` 编译出来的，通过前面的分析，其实我们已经可以大致的猜测出来到底

第一个部分是 pnp optionrom 规范的内容，第二个就是通过 fw_cfg 获取到 kernel image 的地址，然后跳转过去了

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

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
