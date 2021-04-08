# trace 
- [ ] 测试一下, 添加新的 Trace 的方法

- [ ]  为什么会支持各种 backend, 甚至有的 dtrace 的内容?

- [ ] 这里显示的 trace 都是做什么的 ?
```
➜  vn git:(master) ✗ qemu-system-x86_64 -trace help
```

- [ ] 例如这些文件:
/home/maritns3/core/qemu/hw/i386/trace-events

# log
util/log.c 定义所有的 log, 其实整个机制是很容易的

- asm_in : accel/tcg/translator.c::translator_loop
- asm_out : tcg/translate-all.c::tb_gen_code
