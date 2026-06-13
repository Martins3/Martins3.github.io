# chipyard 环境搭建

```sh
git clone https://github.com/ucb-bar/chipyard.git
cd chipyard
# 将各种依赖全部都搞下来
./scripts/init-submodules-no-riscv-tools.sh
```
也使用 rocket.nix 配置文件

## 尝试在 Ubuntu 20.04 中安装
安装依赖的时候，没有什么添加 Ubuntu 的额外源
- scala 使用 nvim-metals 的教程添加
- git 版本在 20.04 中已经满足

## questions
- [ ] chipyard 为什么是需要 QEMU 的啊
- [ ] hwacha 是做什么的?

## 基本使用方法
```sh
source env.sh
cd sims/verilator
make CONFIG=SmallBoomConfig
```

运行测试
```sh
./simulator-chipyard-SmallBoomConfig $RISCV/riscv64-unknown-elf/share/riscv-tests/isa/rv64ui-p-simple
```

运行自己的测试[^1]
```sh
riscv64-unknown-elf-gcc -fno-common -fno-builtin-printf -specs=htif_nano.specs -c hello.c
riscv64-unknown-elf-gcc -static -specs=htif_nano.specs hello.o -o hello.riscv
spike hello.riscv
```

## neovim 环境搭建
现有的 neovim 配置开箱即用，无需任何额外的配置。

- [ ] 搞一个截图出来说明一下

参考: https://github.com/ucb-bar/chipyard/issues/986

在根目录上运行
```sh
sbt bloopInstall
```

## [ ] 主要组成成分是什么
```txt
#  Skipping submodule 'generators/ara'
#  Skipping submodule 'generators/compress-acc'
#  Skipping submodule 'generators/cva6'
#  Skipping submodule 'generators/gemmini'
#  Skipping submodule 'generators/nvdla'
#  Skipping submodule 'generators/rocket-chip'
#  Skipping submodule 'generators/vexiiriscv'
#  Skipping submodule 'sims/firesim'
#  Skipping submodule 'software/coremark'
#  Skipping submodule 'software/firemarshal'
#  Skipping submodule 'software/nvdla-workload'
#  Skipping submodule 'software/spec2017'
#  Skipping submodule 'toolchains/libgloss'
#  Skipping submodule 'toolchains/riscv-tools/riscv-isa-sim'
#  Skipping submodule 'toolchains/riscv-tools/riscv-openocd'
#  Skipping submodule 'toolchains/riscv-tools/riscv-pk'
#  Skipping submodule 'toolchains/riscv-tools/riscv-spike-devices'
#  Skipping submodule 'toolchains/riscv-tools/riscv-tests'
#  Skipping submodule 'toolchains/riscv-tools/riscv-tools-feedstock'
#  Skipping submodule 'tools/circt'
#  Skipping submodule 'tools/dsptools'
#  Skipping submodule 'tools/rocket-dsp-utils'
#  Skipping submodule 'vlsi/hammer-mentor-plugins'
```

## ice nic
- https://github.com/firesim/icenet/blob/master/src/main/scala/DMA.scala
- https://github.com/firesim/icenet-driver/blob/master/icenet.c

就这个驱动，我看这个就不像是可以实现 200G 的东西。

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
