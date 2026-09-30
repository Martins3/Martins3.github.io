## opcode_table 的使用位置

```c
static const struct opcode opcode_table[256];

static const struct opcode twobyte_table[256];
```

```c
struct opcode {
    u64 flags : 56;
    u64 intercept : 8;
    union {
        int (*execute)(struct x86_emulate_ctxt *ctxt);
        const struct opcode *group;
        const struct group_dual *gdual;
        const struct gprefix *gprefix;
        const struct escape *esc;
        const struct instr_dual *idual;
        const struct mode_dual *mdual;
        void (*fastop)(struct fastop *fake);
    } u;
    int (*check_perm)(struct x86_emulate_ctxt *ctxt);
};
```


## emulate_ops 只有一个调用，为什么做成回调?
就当前代码而言，回调不是功能上的必需，改成直接函数调用也是可行的。 这张表的价值主要在于表达模拟器与执行环境之间的接口。

例如 arch/x86/kvm/emulate.c:1320：

rc = ctxt->ops->read_emulated(ctxt, addr, buffer, size,
                             &ctxt->exception);

模拟器负责解码指令、计算地址、处理操作数；这个接口负责“从 guest 地址读取数据”。地址翻译、跨页访问、MMIO、退出用户态等细节由 x86.c
处理。把这些操作集中在 ops 中，可以明确看到模拟器需要执行环境提供哪些能力。

不过，分层本身并不要求函数指针。 普通函数接口同样可以保留这个边界。当前只有一个实现，使用 ops 确实增加了一层间接调用，收益需要结合
历史理解：文件开头 (arch/x86/kvm/emulate.c:1) 明确标注模拟器源自 Xen，并将它描述为通用 x86 指令解码器和模拟器，这种接口形式与可复用
组件的设计相符。

因此，更准确的说法是：这是保留下来的接口抽象；当前树没有利用它切换实现。 而且现在 emulate.c 已经包含 KVM 专用头文件，也不能仅凭这张
ops 表就认为模拟器完全独立于 KVM。

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
