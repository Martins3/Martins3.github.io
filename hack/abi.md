## System V Application Binary Interface

### 3.2 Function Calling Sequence
The AMD64 architecture provides **16** general purpose 64-bit registers.
In addition the architecture provides **16** SSE registers, each 128 bits wide
and **8** x87 floating point registers, each 80 bits wide. 

This subsection discusses usage of each register. Registers `%rbp`, `%rbx` and
`%r12` through `%r15` “belong” to the calling function and the called function is
required to preserve their values. In other words, a called function must preserve
these registers’ values for its caller. Remaining registers “belong” to the called
function. If a calling function wants to preserve such a register value across a function call, it must save the value in its local stack frame

> 大多數的時候，children possess 根本不會使用到那麼多的參數，所以在編譯的時候，沒有保存
