## pnp
似乎 pnp 是 acpi 的重要组成部分



从这里面，其实可以非常清楚的 trace 在 acpi table 中 page walk 的时候，将这种资源添加进去的，其中包括中断号资源
在 serial_pnp_probe 中，将这个中断号告知设备驱动，之后再去注册。
```txt
#0  pnp_add_resource (dev=dev@entry=0xffff888100322400, res=res@entry=0xffffc90000013c78) at drivers/pnp/resource.c:514
#1  0xffffffff814af932 in pnpacpi_allocated_resource (res=0xffff88810033da40, data=0xffff888100322400) at drivers/pnp/pnpacpi/rsparser.c:177
#2  0xffffffff81497cb0 in acpi_walk_resource_buffer (buffer=buffer@entry=0xffffc90000013d18, user_function=user_function@entry=0xffffffff814af8e0 <pnpacpi_allocated_res ource>, context=context@entry=0xffff888100322400) at drivers/acpi/acpica/rsxface.c:547
#3  0xffffffff814980ce in acpi_walk_resources (context=0xffff888100322400, user_function=0xffffffff814af8e0 <pnpacpi_allocated_resource>, name=0xffffffff8225bf9f "_CRS" , device_handle=0xffff888100322400) at drivers/acpi/acpica/rsxface.c:623
#4  acpi_walk_resources (device_handle=device_handle@entry=0xffff8881000f2780, name=name@entry=0xffffffff8225bf9f "_CRS", user_function=user_function@entry=0xffffffff81 4af8e0 <pnpacpi_allocated_resource>, context=context@entry=0xffff888100322400) at drivers/acpi/acpica/rsxface.c:594
#5  0xffffffff814afb90 in pnpacpi_parse_allocated_resource (dev=dev@entry=0xffff888100322400) at drivers/pnp/pnpacpi/rsparser.c:280
#6  0xffffffff82b80ea4 in pnpacpi_add_device (device=0xffff8881001eb800) at drivers/pnp/pnpacpi/core.c:258
#7  pnpacpi_add_device_handler (handle=<optimized out>, lvl=<optimized out>, context=<optimized out>, rv=<optimized out>) at drivers/pnp/pnpacpi/core.c:295
#8  0xffffffff81493756 in acpi_ns_get_device_callback (return_value=0x0 <fixed_percpu_data>, context=0xffffc90000013e80, nesting_level=4, obj_handle=0xffff8881000f2780) at drivers/acpi/acpica/nsxfeval.c:740
#9  acpi_ns_get_device_callback (obj_handle=obj_handle@entry=0xffff8881000f2780, nesting_level=nesting_level@entry=4, context=context@entry=0xffffc90000013e80, return_v alue=return_value@entry=0x0 <fixed_percpu_data>) at drivers/acpi/acpica/nsxfeval.c:635
#10 0xffffffff81492fe5 in acpi_ns_walk_namespace (type=type@entry=6, start_node=<optimized out>, start_node@entry=0xffffffffffffffff, max_depth=max_depth@entry=42949672 95, flags=flags@entry=1, descending_callback=descending_callback@entry=0xffffffff814935e0 <acpi_ns_get_device_callback>, ascending_callback=ascending_callback@entry=0x0 <fixed_percpu_data>, context=0xffffc90000013e80, return_value=0x0 <fixed_percpu_data>) at drivers/acpi/acpica/nswalk.c:229
#11 0xffffffff81493152 in acpi_get_devices (HID=HID@entry=0x0 <fixed_percpu_data>, user_function=user_function@entry=0xffffffff82b80d07 <pnpacpi_add_device_handler>, co
ntext=context@entry=0x0 <fixed_percpu_data>, return_value=return_value@entry=0x0 <fixed_percpu_data>) at drivers/acpi/acpica/nsxfeval.c:805
#12 0xffffffff82b80ce8 in pnpacpi_init () at drivers/pnp/pnpacpi/core.c:308
#13 0xffffffff81000def in do_one_initcall (fn=0xffffffff82b80ca2 <pnpacpi_init>) at init/main.c:1249
#14 0xffffffff82b4c26b in do_initcall_level (command_line=0xffff888100123400 "root", level=5) at ./include/linux/compiler.h:234
#15 do_initcalls () at init/main.c:1338
#16 do_basic_setup () at init/main.c:1358
#17 kernel_init_freeable () at init/main.c:1560
#18 0xffffffff81b97a69 in kernel_init (unused=<optimized out>) at init/main.c:1447
#19 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
#20 0x0000000000000000 in ?? ()
```

键盘的中断号也是通过 pnp 来进行设置的：
1. i8042_pnp_kbd_probe 会设置 i8042_pnp_kbd_irq
2. i8042_pnp_init 用 i8042_pnp_kbd_probe 来设置 i8042_kbd_irq，然后会拿着这个数值去 request_irq

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
