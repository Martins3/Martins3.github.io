## TODO

1. module_init() and module_exit()
2. ignore_loglevel

3. why pr_debug() can print to the terminal ?
    1. what's the difference with printk ?

4. why we can corrupt /tmp/tmp.QXJzapOmjJ ?  What's that ?

http://tldp.org/LDP/lkmpg/2.6/html/x323.html

5. https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=0b999ae3614d09d97a1575936bcee884f912b10e
```c
static int __init cmd_init(void)
{
	pr_info("Early bird gets %s\n", str);
	return 0;
}
```

6. we are not able to boot : /home/shen/Core/hack-linux-kernel/tools/labs/core-image-sato-qemux86.ext4

7. extra exercise 1 4 5 : didn't worked
    1. echo hvc0 > /sys/module/kgdboc/parameters/kgdboc :
    2. echo g > /proc/sysrq-trigger
    3. ctrl+O g
    4. /debug/dynamic_debug/control is protected from writing !

8. pr_info 和 printk 的区别是什么 ?
