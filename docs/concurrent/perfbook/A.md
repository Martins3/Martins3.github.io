1. 为什么 parallel 可能更慢?
  - [ ] 3 5.1 和 6 中给出了答案
  - 4.1 说 parallel 的 positive 的例子
2. 14.2 说 lockless 中做的更好
  - [ ] Figure 5.1 中，lockless
  - as shown in Table 17.3 on page 392.
Section 14.2 looks more deeply at non-blocking synchronization, which is a popular lockless methodology

 This is not
a bad thing, considering that on modern computer systems,
the program counter is a truly horrible clock [MOZ09]
> 什么意思？

 Unfortunately,
for all of these counters, ordering against all effects of
prior and subsequent code requires expensive memorybarrier instructions
> 为什么会

11.7 验证什么

- [ ]
