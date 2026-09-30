## 工具
- https://developer.arm.com/documentation/102142/latest/
- https://esr.arm64.dev/
- https://salmanarif.bitbucket.io/visual/index.html : 模拟器

## 官方文档
- https://learn.arm.com/
- https://github.com/ArmDeveloperEcosystem
  - https://developer.arm.com/documentation/ihi0062/b/The-Translation-Process/Translation-and-protection-checks/Translation-table-format
  - https://learn.arm.com/
- https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst : ABI 官方文档

## 指令速查手册
- https://courses.cs.washington.edu/courses/cse469/19wi/arm64.pdf : 常用的版本
- https://pages.cs.wisc.edu/~markhill/restricted/arm_isa_quick_reference.pdf : 各种寻址的总结
- https://www.usna.edu/Users/cs/lmcdowel/courses/ic220/S20/resources/ARM-v8-Quick-Reference-Guide.pdf : 侧重编码的部分
- https://www.cs.princeton.edu/courses/archive/spr22/cos217/reading/ArmInstructionSetOverview.pdf : 一个详细的速查手册

## 常用问题
### ABI
```txt
r0-r3 are the argument and scratch registers; r0-r1 are also the result registers
r4-r8 are callee-save registers
r9 might be a callee-save register or not (on some variants of AAPCS it is a special register)
r10-r11 are callee-save registers
r12-r15 are special registers
```

## 高级
1. psci 框架 : https://www.cnblogs.com/loyenwang/p/11370557.html
  - 和 acpi 什么关系
2. SMCCC : https://developer.arm.com/Architectures/Secure%20Monitor%20Calling%20Convention
3. https://github.com/TheAlgorithms/AArch64_Assembly : the Algorithm 的 ARM 实现，没必要看了
4. https://eocanha.org/blog/2020/10/16/figuring-out-corrupt-stacktraces-on-arm/ : arm 的调试技术
5. https://people.kernel.org/linusw/how-the-arm32-kernel-starts : 启动分析

在 x86 上使用 acpi, 在 arm 上使用 dtb 来描述设备的逻辑关系


## ARM 本身的关键参考
- [ARM 指令集导读](https://armv8-ref.codingbelief.com/zh/)

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
