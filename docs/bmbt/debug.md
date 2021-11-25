> 只是一个 backup

## some new bug

```c
guest ip : ee0ef
failed in [memory dispatch] with offset=[fee000f0]
```

## PIIX4_PM memory mappings，至少验证一下这个设备是否真的有用
我们需要 PIIX4_PM 设备吗

## 根本没有实现 smbios 啊，只是在拷贝啥啊
Copying SMBIOS entry point from 0x00006e30 to 0x000f5b00

## TSC
tsctimer_setup

更加简答的做法是，不要提供
CPUID_TSC 出来了，太 TM 的痛苦了

- [ ] 实际上，在 64bit 的机器中，根本没有 tsctimer_setup 的操作
- [ ] 但是 TCG_FEATURES 中确实含有 CPUID_TSC，为何可以绕过

## 可以屏蔽 keyboard 吗
可以的

## 调查 vga 输出的效果
seabios 的输出中间，现在可以看到:

Scan for VGA option rom
No VGA found, scan for other display
Turning on vga text mode console
SeaBIOS (version rel-1.14.0-14-g748d619-dirty-20211118_102229-maritns3-pc)

可以需要具体分析一下 seabios handler 的效果:
```c
// Call int10 vga handler.
static void
call16_int10(struct bregs *br)
{
    br->flags = F_IF;
    start_preempt();
    call16_int(0x10, br);
    finish_preempt();
}
```
反正之后就可以使用 printf 了，但是具体是怎么做的不知道

## load linux kernel
- [ ] MachineClass::default_boot_order; 的作用是什么?

- [ ] 是如何处理 initrd 的
  - 实际上这个可以参数不去处理
- [ ] FW_CFG_SETUP_ADDR / FW_CFG_SETUP_SIZE / FW_CFG_SETUP_DATA 到底是怎么使用的
  - 处理什么 real_addr / header / setup 之类的操作都是啥啊

### 为什么会出现 interrupt block 的错误
```c
bmbt.bin: src/util/qemu-timer.c:435: timer_interrupt_handler: Assertion `!is_interrupt_blocked()' failed.
```

- [ ] 为什么存在如此高频的中断啊
- [ ] 将信息导入到 bmbt.log 中，感觉模式很强的


### target_x86_to_mips_host 的参数 max_insns 替换为 1 那么 seabios 会出现错误
在足够长的时间的等待之后，会出现在:
```c
huxueshi:tb_find 70716
huxueshi:tb_find 70718
bmbt.bin: src/util/qemu-timer.c:435: timer_interrupt_handler: Assertion `!is_interrupt_blocked()' failed.
```
