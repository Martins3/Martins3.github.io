- https://superuser.com/questions/1283788/what-exactly-is-microcode-and-how-does-it-differ-from-firmware
  - firmware 不一定是放到 ROM 中的，也可能直接在软件中
  - microcode 是 firmware 的一种
  - 修改 CPU 的指令映射
- https://winraid.level1techs.com/t/intel-amd-via-freescale-cpu-microcode-repositories-discussion/32301
  - 这里包含具体的资源

代码在这里
arch/x86/kernel/cpu/microcode/
