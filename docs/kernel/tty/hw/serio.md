# 
drivers/input/serio/i8042.c

# tty driver

https://en.wikipedia.org/wiki/COM_(hardware_interface)

During the initialization function (init_module()), or in the function that opens the device, interrupts must be activated for the device. This operation is dependent on the device, but most often involves setting a bit from the control register.

Because the interrupt handlers run in interrupt context the actions that can be performed are limited: unable to access user space memory, can’t call blocking functions.
**Also synchronization using spinlocks is tricky and can lead to deadlocks if the spinlock used is already acquired by a process that has been interrupted by the running handler.**

IFF you know that the spinlocks are never used in interrupt handlers, you can use the non-irq versions.

If we want to disable interrupts at the interrupt controller level (not recommended because disabling a particular interrupt is slower, we can not disable shared interrupts) we can do this with disable_irq(), disable_irq_nosync(), and enable_irq(). Using these functions will disable the interrupts on all processors.


```c
static int __init i8042_setup_kbd(void)
{
    int error;

    error = i8042_create_kbd_port();
    if (error)
        return error;

    error = request_irq(I8042_KBD_IRQ, i8042_interrupt, IRQF_SHARED,
                "i8042", i8042_platform_device);
    if (error)
        goto err_free_port;

    error = i8042_enable_kbd_port(); // tell the device, everything is ok, start working now !
    if (error)
        goto err_free_irq;

    i8042_kbd_irq_registered = true;
    return 0;

 err_free_irq:
    free_irq(I8042_KBD_IRQ, i8042_platform_device);
 err_free_port:
    i8042_free_kbd_port();
    return error;
}


/*
 * i8042_interrupt() is the most important function in this driver -
 * it handles the interrupts from the i8042, and sends incoming bytes
 * to the upper layers.
 */

static irqreturn_t i8042_interrupt(int irq, void *dev_id)
// /home/shen/Core/linux/drivers/input/serio/i8042.c
// XXX although very important, but it's fairly simple
// read data by i8042_read_data

static inline int i8042_read_data(void)
{
    return inb(I8042_DATA_REG);
}
```

1. [](https://en.wikipedia.org/wiki/Keyboard_controller_(computing))
2. https://wiki.osdev.org/%228042%22_PS/2_Controller#Initialising_the_PS.2F2_Controller

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
