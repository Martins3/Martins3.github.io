# chipyard 环境搭建

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

## Learn the basic syntax

### Class
> primary constructor
> all kinds of getter and setter

## Object
Use an object in Scala whenever you would have used a singleton object in Java
or C++:
• As a home for utility functions or constants
• When a single immutable instance can be shared efficiently
• When a single instance is required to coordinate some service (the singleton
design pattern)

The class and its companion object can access each other’s private features. They
must be located in the same source file

[class and object in scala](https://stackoverflow.com/questions/1755345/difference-between-object-and-class-in-scala)
[implict class](https://stackoverflow.com/questions/40878893/implicit-classes-in-scala)
[case class](https://stackoverflow.com/questions/5270752/difference-between-case-object-and-object)
[case object](https://madusudanan.com/blog/scala-tutorials-part-10-case-objects-in-scala/)

## questions
1. parameters of class
```scala
class Animal(val name: String, val species: String)
class Human(val name: String)
implicit def human2animal(h: Human): Animal = {
    new Animal(h.name, "Homo sapiens")
}
val me = new Human("Adam")
println(me.species)
```

## Inherience
> Superclass Construction

#### Use Vim
http://seenaburns.com/vim-setup-for-scala/

1. Scala functions that do not have any arguments do not require empty parenthese
2. By convention, argument-less functions that do not have side effects (i.e. calling them does not change anything and they simply return a value) do not use parentheses, and functions that do have side effects (perhaps they change class variables or print stuff out) should require parentheses.
3. Overloaded functions should be avoided, though.

[^1]: https://chipyard.readthedocs.io/en/latest/Software/Baremetal.html
