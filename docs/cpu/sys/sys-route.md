# 计划就是，首先理解软件层的指令集的含义，然后再去处理怎么写

# blog

https://semiengineering.com/why-its-so-hard-to-create-new-processors/
https://github.com/EugeneLiu/translationCSAPP : csapp 的翻译项目

## riscv
- https://github.com/ucb-bar/midas : 将 chisel 编译到 verilog，估计不咋地
- https://github.com/freechipsproject/chisel-bootcamp : chisel 学习基础，质量应该是很高的
- https://github.com/freechipsproject/chisel-template : 教学
- https://github.com/mortbopet/Ripes : 给小学生学习 RISC-V 的图形化模拟器
- https://github.com/SpinalHDL/VexRiscv : 一个 Riscv FPGA 的实现
- https://github.com/SI-RISCV/e200_opensource : 蜂鸟处理器
  - https://github.com/riscv-mcu/e203_hbirdv2
- https://github.com/riscv-boom/riscv-boom : boom v3 的版本应该是非常复杂的了
- https://github.com/chipsalliance/rocket-chip
- https://github.com/ZipCPU/zipcpu : 搞了一个自己的指令集发
- https://github.com/riscv/riscv-isa-sim : 指令学习好帮手
- https://github.com/riscv/riscv-isa-manual : 指令手册
- https://github.com/sergeykhbr/riscv_vhdl : SOC，包括各种完整的项目
- https://github.com/darklife/darkriscv : 一晚上实现
- https://github.com/openrisc/mor1kx
- https://github.com/SymbioticEDA/riscv-formal : 验证框架
- https://github.com/cliffordwolf/picorv32 : 又一个核
- https://news.ycombinator.com/item?id=41479637 : 有趣，评价不错，luajit 的 RISC-V 实现

## 辅助工具

https://github.com/andrescv/Jupiter : RISC-V assembler and runtime simulator 停止开发了很久了

https://github.com/piotte13/SIMD-Visualiser : 帮助理解 SIMD 的网站

https://github.com/mortbopet/Ripes : A graphical processor simulator and assembly editor for the RISC-V ISA Topics

https://github.com/riscv-software-src/riscv-isa-sim : spike

## 教程

https://github.com/JonnyKong/CMU-15-213-Intro-to-Computer-Systems : CSAPP 的试验

- [ ] https://news.ycombinator.com/item?id=25257932 makes me want to understand how CPU works again

- https://avrillion.com/stf/363/How-to-Build-1-Bit-of-RAM-Using-Transistors : 制造一个 bit 的 RAM
- https://en.wikipedia.org/wiki/Dennard_scaling
- https://github.com/YosysHQ/yosys

## 这里也是有 zynq 的代码

https://github.com/rsd-devel/rsd/blob/master/Processor/Src/Main_Zynq.sv

其实其中的 wiki 也是不错的:
https://github.com/rsd-devel/rsd/wiki/en-home

## 这个有用吗?
https://ysyx.oscc.cc/signup/

## pikvm ，有趣
https://github.com/pikvm/pikvm


https://essenceia.github.io/projects/ethernet_switch_asic/

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
