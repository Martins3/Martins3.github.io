# QEMU 执行的 guest 第一行代码

## seabios 一侧
> On emulators, this phase starts when the CPU starts execution in 16bit
> mode at 0xFFFF0000:FFF0. The emulators map the SeaBIOS binary to this
> address, and SeaBIOS arranges for romlayout.S:reset_vector() to be
> present there. This code calls romlayout.S:entry_post() which then
> calls post.c:handle_post() in 32bit mode.

以上是 seabios 的文档，意思很简单: reset_vector => entry_post => handle_post

从 seabios 的源码中也可以验证:
```asm
        // Reset stack, transition to 32bit mode, and call a C function.
        .macro ENTRY_INTO32 cfunc
        xorw %dx, %dx
        movw %dx, %ss
        movl $ BUILD_STACK_ADDR , %esp
        movl $ \cfunc , %edx
        jmp transition32
        .endm
```

```asm
entry_post:
        cmpl $0, %cs:HaveRunPost                // Check for resume/reboot
        jnz entry_resume
        ENTRY_INTO32 _cfunc32flat_handle_post   // Normal entry point

        ORG 0xe2c3
```

```asm
reset_vector:
        ljmpw $SEG_BIOS, $entry_post

        // 0xfff5 - BiosDate in misc.c

        // 0xfffe - BiosModelId in misc.c

        // 0xffff - BiosChecksum in misc.c

        .end
```
从上面的代码还可以知道，stack 的顶是 0x7000
```c
#define BUILD_STACK_ADDR          0x7000
```


在 seabios 添加一个调试语句
```c
dprintf(1, "%p\n", VSYMBOL(entry_post));
```
可以很容易得到 entry_post 的地址为: `0x000fe05b`

也就是 seabios 执行的第一行代码就是从 0xfffffff0 跳转到 0x000fe05b 上

## QEMU 一侧
主要发生在 loader.c 中

### 构建 pc.bios 地址空间
在 x86_bios_rom_init 中初始化，这个没有什么好分析的:

```c
void x86_bios_rom_init(MemoryRegion *rom_memory, bool isapc_ram_fw)
{
    bios = g_malloc(sizeof(*bios));
    memory_region_init_ram(bios, NULL, "pc.bios", bios_size, &error_fatal);
    if (!isapc_ram_fw) {
        memory_region_set_readonly(bios, true);
    }
    ret = rom_add_file_fixed(bios_name, (uint32_t)(-bios_size), -1);
```

### 关联 Rom image
创建 MemoryRegion pc.bios 但是如何将 Rom image 关联上去值得分析:


## loader
- isa-bios 是 pc.bios 的后 128k 的部分，而且放到 1M 的后面的空间中

总体来说，loader 在处理 elf, ramdisk 和 rom 的事情，但是暂时需要的并不多。

## struct Rom

体现在 rom_add_file 中, 会调用 fw_cfg_add_file, 将文件添加进去


## 问题
- 虽然从代码中可以知道 pc Machine 的 0 ~ 1M 的物理地址空间内的各种布局，但是其规定在哪里，并不知道是来自于哪一个手册?
- 为什么需要将 pc.bios 映射到 4G pci 空间的最上方，这又是哪里的规定?

[^1]: https://en.wikipedia.org/wiki/Cylinder-head-sector
