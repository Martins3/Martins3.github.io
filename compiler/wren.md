# wren

## 问题
- [ ] 什么是 Fiber
- [ ] initcore 为什么需要调用那么长的链条
- [ ] 是基于 stackbased 的了

## 一些直觉的记录
- matchLine : 似乎编译一行
  - match
    - nextToken : 词法分析器
      - skipLineComment / skipBlockComment
- consume

- interpolation 是如何实现的?
```wren
System.print("Math %(3 + 4 * 5) is fun!") //> Math 23 is fun!
```

- ObjFn
- ObjModule

- emitByte : 之后，如何操作?

## 分析一下 `wren_vm` 和 `wren_compile` 是如何链接起来的

- wrenInterpret : 会被调用两次的

第一次:
```txt
#0  wrenInterpret (vm=0x55555558e2a0, module=0x55555558e880 "", source=0x9555828bd <error: Cannot access memory at address 0x
9555828bd>) at ../../src/vm/wren_vm.c:1517
#1  0x00005555555717b6 in wrenInitializeCore (vm=0x55555558e2a0) at ../../src/vm/wren_core.c:1299
#2  0x000055555555e11e in wrenNewVM (config=0x7fffffffd490) at ../../src/vm/wren_vm.c:96
#3  0x000055555555a19e in initVM (isAPITest=false) at ../../test/main.c:31
#4  0x000055555555a248 in main (argc=2, argv=0x7fffffffd648) at ../../test/main.c:43
```

第二次
```txt
#0  wrenInterpret (vm=0x5555555a19b0, module=0x5555555bf260 "System.print(\"hello wren\")\nSystem.print(\"\\U0001F64A\\U0001F
680\")\n", source=0x55555555ca29 <pathRemoveExtension+179> "\311\303\363\017\036\372UH\211\345H\203\354\020H\211}\370\211\360
\210E\364H\213E\bH\211\306H\215=\335\377\377\377蝬\377\377H\213E\370H\213@\bH\215P\001H\213E\370H\211\326H\211\307\350\027\37
2\377\377H\213E\370H\213\060H\213E\370H\213@\bH\215H\001H\213U\370H\211J\bH\215\024\006\017\266E\364\210\002H\213E\370H\213\0
20H\213E\370H\213@\bH\001\320", <incomplete sequence \306>) at ../../src/vm/wren_vm.c:1517
#1  0x000055555555d628 in runFile (vm=0x55555558e2a0, path=0x7fffffffdad8 "../test.wren") at ../../test/test.c:456
#2  0x000055555555a265 in main (argc=2, argv=0x7fffffffd648) at ../../test/main.c:44
```

应该这两步吧:
```c
WrenInterpretResult wrenInterpret(WrenVM* vm, const char* module,
                                  const char* source)
{
  ObjClosure* closure = wrenCompileSource(vm, module, source, false, true);
  if (closure == NULL) return WREN_RESULT_COMPILE_ERROR;

  wrenPushRoot(vm, (Obj*)closure);
  ObjFiber* fiber = wrenNewFiber(vm, closure);
  wrenPopRoot(vm); // closure.
  vm->apiStack = NULL;

  return runInterpreter(vm, fiber);
}
```

## 核心框架

```txt
../test: (script)
   1: 0000  LOAD_MODULE_VAR     22 'System'
      0003  CONSTANT             0 'hello wren'
      0006  CALL_1              79 'print(_)'
      0009  POP
   2: 0010  LOAD_MODULE_VAR     22 'System'
      0013  CONSTANT             1 '🙊🚀'
      0016  CALL_1              79 'print(_)'
      0019  POP
   3: 0020  END_MODULE
      0021  RETURN
      0022  END
```
这个东西是如何被编译出来的，然后是如何被执行的?


- [ ] 感觉 `wren_vm` 中的函数从来没有被运行过啊

- callMethod
- call
- call_fn

```txt
#0  callMethod (compiler=0x5555555670a8 <loadLocal+47>, numArgs=32767, name=0x7fffffff8da0 "`\257\377\377\377\177", length=-2
9632) at ../../src/vm/wren_compiler.c:1997
#1  0x000055555556979a in forStatement (compiler=0x7fffffff8da0) at ../../src/vm/wren_compiler.c:3098
#2  0x0000555555569b2e in statement (compiler=0x7fffffff8da0) at ../../src/vm/wren_compiler.c:3214
#3  0x000055555556ade2 in definition (compiler=0x7fffffff8da0) at ../../src/vm/wren_compiler.c:3758
#4  0x0000555555567375 in finishBlock (compiler=0x7fffffff8da0) at ../../src/vm/wren_compiler.c:1790
#5  0x00005555555673e5 in finishBody (compiler=0x7fffffff8da0) at ../../src/vm/wren_compiler.c:1805
#6  0x000055555556a594 in method (compiler=0x7fffffffb020, classVariable=...) at ../../src/vm/wren_compiler.c:3491
#7  0x000055555556a8bb in classDefinition (compiler=0x7fffffffb020, isForeign=false) at ../../src/vm/wren_compiler.c:3592
#8  0x000055555556ad3f in definition (compiler=0x7fffffffb020) at ../../src/vm/wren_compiler.c:3736
#9  0x000055555556afb5 in wrenCompile (vm=0x55555558e2a0, module=0x55555558e470, source=0x55555557ff60 "class Bool {}\nclass
Fiber {}\nclass Fn {}\nclass Null {}\nclass Num {}\n\nclass Sequence {\n  all(f) {\n    var result = true\n    for (element i
n this) {\n      result = f.call(element)\n      if (!result) ret"..., isExpression=false, printErrors=true) at ../../src/vm/
wren_compiler.c:3809
#10 0x000055555555ec91 in compileInModule (vm=0x55555558e2a0, name=9222246136947933185, source=0x55555557ff60 "class Bool {}\
nclass Fiber {}\nclass Fn {}\nclass Null {}\nclass Num {}\n\nclass Sequence {\n  all(f) {\n    var result = true\n    for (el
ement in this) {\n      result = f.call(element)\n      if (!result) ret"..., isExpression=false, printErrors=true) at ../../
src/vm/wren_vm.c:485
#11 0x0000555555562417 in wrenCompileSource (vm=0x55555558e2a0, module=0x0, source=0x55555557ff60 "class Bool {}\nclass Fiber
 {}\nclass Fn {}\nclass Null {}\nclass Num {}\n\nclass Sequence {\n  all(f) {\n    var result = true\n    for (element in thi
s) {\n      result = f.call(element)\n      if (!result) ret"..., isExpression=false, printErrors=true) at ../../src/vm/wren_
vm.c:1539
#12 0x000055555556231b in wrenInterpret (vm=0x55555558e2a0, module=0x0, source=0x55555557ff60 "class Bool {}\nclass Fiber {}\
nclass Fn {}\nclass Null {}\nclass Num {}\n\nclass Sequence {\n  all(f) {\n    var result = true\n    for (element in this) {
\n      result = f.call(element)\n      if (!result) ret"...) at ../../src/vm/wren_vm.c:1518
#13 0x000055555557175b in wrenInitializeCore (vm=0x55555558e2a0) at ../../src/vm/wren_core.c:1299
#14 0x000055555555e11e in wrenNewVM (config=0x7fffffffd490) at ../../src/vm/wren_vm.c:96
#15 0x000055555555a19e in initVM (isAPITest=false) at ../../test/main.c:31
#16 0x000055555555a248 in main (argc=2, argv=0x7fffffffd648) at ../../test/main.c:43
```
感觉什么 statement 都会成为 forStatement

并不是，之前之前存在大量的准备工作而已。

## [ ] 写一个 wren 的 tressitter 以及 lsp 吧，如果可以的话
```wren
# DURATION     TID     FUNCTION
            [180426] | main() {
            [180426] |   handle_args() {
   3.901 us [180426] |     strcmp();
   5.186 us [180426] |   } /* handle_args */
            [180426] |   isModuleAnAPITest() {
   0.123 us [180426] |     strncmp();
   0.065 us [180426] |     strncmp();
   0.666 us [180426] |   } /* isModuleAnAPITest */
            [180426] |   initVM() {
   0.143 us [180426] |     memset();
   0.219 us [180426] |     strncmp();
   5.674 ms [180426] |   } /* initVM */
            [180426] |   runFile() {
            [180426] |     readFile() {
   8.060 us [180426] |       fopen();
   9.221 us [180426] |       fseek();
   0.278 us [180426] |       ftell();
   0.941 us [180426] |       rewind();
   0.282 us [180426] |       malloc();
   0.410 us [180426] |       fread();
   2.814 us [180426] |       fclose();
  23.534 us [180426] |     } /* readFile */
            [180426] |     pathNew() {
   0.109 us [180426] |       malloc();
   0.078 us [180426] |       malloc();
            [180426] |       pathAppendString() {
            [180426] |         appendSlice() {
            [180426] |           ensureCapacity() {
   0.444 us [180426] |           } /* ensureCapacity */
   0.905 us [180426] |         } /* appendSlice */
   1.330 us [180426] |       } /* pathAppendString */
   1.982 us [180426] |     } /* pathNew */
            [180426] |     pathType() {
            [180426] |       absolutePrefixLength() {
   0.066 us [180426] |         isSeparator();
   0.306 us [180426] |       } /* absolutePrefixLength */
   0.051 us [180426] |       isSeparator();
   0.051 us [180426] |       isSeparator();
   0.762 us [180426] |     } /* pathType */
            [180426] |     pathRemoveExtension() {
   0.051 us [180426] |       isSeparator();
   0.037 us [180426] |       isSeparator();
   0.038 us [180426] |       isSeparator();
   0.037 us [180426] |       isSeparator();
   0.037 us [180426] |       isSeparator();
   0.036 us [180426] |       isSeparator();
   0.038 us [180426] |       isSeparator();
   0.038 us [180426] |       isSeparator();
   0.036 us [180426] |       isSeparator();
   0.050 us [180426] |       isSeparator();
   1.426 us [180426] |     } /* pathRemoveExtension */
   0.116 us [180426] |     strncmp();
            [180426] |     vm_write() {
   2.608 us [180426] |       printf();
   2.916 us [180426] |     } /* vm_write */
            [180426] |     vm_write() {
   0.361 us [180426] |       printf();
   0.548 us [180426] |     } /* vm_write */
            [180426] |     pathFree() {
   0.541 us [180426] |     } /* pathFree */
  84.703 us [180426] |   } /* runFile */
   6.319 ms [180426] | } /* main */
```

## [ ]  使用 uftrace 的工具首先简单的跑一下

- 使用 uftrace 跟踪了一下，感觉 memcpy / memcpy 之类的函数调用的好频繁啊

## 这个是有一个 bug 的
- /home/maritns3/hack/wren/src/vm/wren_vm.c


## 还是 gdb 调试吧

- [ ] 为什么需要编译两次啊
```txt
#0  compileInModule (vm=0x555500000001, name=140737488343264, source=0x55555558e2a0 "", isExpression=85, printErrors=240) at
../../src/vm/wren_vm.c:456
#1  0x0000555555562417 in wrenCompileSource (vm=0x55555558e2a0, module=0x0, source=0x55555557ff60 "class Bool {}\nclass Fiber
 {}\nclass Fn {}\nclass Null {}\nclass Num {}\n\nclass Sequence {\n  all(f) {\n    var result = true\n    for (element in thi
s) {\n      result = f.call(element)\n      if (!result) ret"..., isExpression=false, printErrors=true) at ../../src/vm/wren_
vm.c:1539
#2  0x000055555556231b in wrenInterpret (vm=0x55555558e2a0, module=0x0, source=0x55555557ff60 "class Bool {}\nclass Fiber {}\
nclass Fn {}\nclass Null {}\nclass Num {}\n\nclass Sequence {\n  all(f) {\n    var result = true\n    for (element in this) {
\n      result = f.call(element)\n      if (!result) ret"...) at ../../src/vm/wren_vm.c:1518
#3  0x000055555557173b in wrenInitializeCore (vm=0x55555558e2a0) at ../../src/vm/wren_core.c:1299
#4  0x000055555555e11e in wrenNewVM (config=0x7fffffffd490) at ../../src/vm/wren_vm.c:96
#5  0x000055555555a19e in initVM (isAPITest=false) at ../../test/main.c:31
#6  0x000055555555a248 in main (argc=2, argv=0x7fffffffd648) at ../../test/main.c:43
```

```txt
#0  compileInModule (vm=0x55555558e2a0, name=93824992550256, source=0x7 <error: Cannot access memory at address 0x7>, isExpre
ssion=false, printErrors=13) at ../../src/vm/wren_vm.c:456
#1  0x0000555555562417 in wrenCompileSource (vm=0x55555558e2a0, module=0x5555555a1d70 "../test", source=0x5555555bee50 "Syste
m.print(\"hello wren\")\n", isExpression=false, printErrors=true) at ../../src/vm/wren_vm.c:1539
#2  0x000055555556231b in wrenInterpret (vm=0x55555558e2a0, module=0x5555555a1d70 "../test", source=0x5555555bee50 "System.pr
int(\"hello wren\")\n") at ../../src/vm/wren_vm.c:1518
#3  0x000055555555d628 in runFile (vm=0x55555558e2a0, path=0x7fffffffdad8 "../test.wren") at ../../test/test.c:456
#4  0x000055555555a265 in main (argc=2, argv=0x7fffffffd648) at ../../test/main.c:44
```
