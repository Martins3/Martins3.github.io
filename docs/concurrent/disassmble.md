# disassable the code

## kernel low level interface

### why
```c
typedef struct {
	int counter;
} atomic_t;

#define ATOMIC_INIT(i) { (i) }
```

### `_raw_spin_lock_irqsave`

## atomic_add_return
```txt
void huxueshi(void)
{
	atomic_t combined_event_count = ATOMIC_INIT(0);
	int x = 12;
	x = atomic_add_return(x, &combined_event_count);
	pr_info("%d", x);
}
```

```txt
dis$ disass huxueshi
Dump of assembler code for function huxueshi:
   0xffffffff81d7a510 <+0>:     endbr64
   0xffffffff81d7a514 <+4>:     sub    $0x10,%rsp
   0xffffffff81d7a518 <+8>:     mov    $0xc,%esi               # 初始化 x
   0xffffffff81d7a51d <+13>:    mov    %gs:0x28,%rax
   0xffffffff81d7a526 <+22>:    mov    %rax,0x8(%rsp)
   0xffffffff81d7a52b <+27>:    xor    %eax,%eax
   0xffffffff81d7a52d <+29>:    movl   $0x0,0x4(%rsp)           # 初始化 combined_event_count
   0xffffffff81d7a535 <+37>:    lock xadd %esi,0x4(%rsp)        # esi 中持有两者之后，而 0x(%rsp) 也就是 atomic_t 被更新上两者之和。

   0xffffffff81d7a53b <+43>:    mov    $0xffffffff82a06bc1,%rdi # 调用 pr_info
   0xffffffff81d7a542 <+50>:    add    $0xc,%esi
   0xffffffff81d7a545 <+53>:    call   0xffffffff82133af8 <_printk>
   0xffffffff81d7a54a <+58>:    mov    0x8(%rsp),%rax
   0xffffffff81d7a54f <+63>:    sub    %gs:0x28,%rax
   0xffffffff81d7a558 <+72>:    jne    0xffffffff81d7a563 <huxueshi+83>
   0xffffffff81d7a55a <+74>:    add    $0x10,%rsp
   0xffffffff81d7a55e <+78>:    jmp    0xffffffff821b86c4 <__x86_return_thunk>
   0xffffffff81d7a563 <+83>:    call   0xffffffff821a50d0 <__stack_chk_fail>
```
实际上就是 xadd(&v->counter, i); 而已。

对比普通的 add :
https://stackoverflow.com/questions/30130752/assembly-does-xadd-instruction-need-lock/65055576#65055576

## spin_lock_irq

```c
static __always_inline void spin_lock_irq(spinlock_t *lock)
{
	raw_spin_lock_irq(&lock->rlock);
}
```

```c
static inline __attribute__((__gnu_inline__)) __attribute__((__unused__)) __attribute__((no_instrument_function)) __attribute__((__always_inline__)) void queued_spin_lock(struct qspinlock *lock)
{
 int val = 0;

 if (__builtin_expect(!!(atomic_try_cmpxchg_acquire(&lock->val, &val, (1U << 0))), 1))
  return;

 queued_spin_lock_slowpath(lock, val);
}
```

## barrier
```c
# define barrier() __asm__ __volatile__("": : :"memory")
```

## qatomic_add
```c
void *trythis(void *arg) {
  unsigned long i = 10000000;
  printf("[huxueshi:%s:%d] \n", __FUNCTION__, __LINE__);
  while (i--) {
    // qatomic_set(&counter, counter + 1);
    qatomic_add(&counter, 1);
  }
  return NULL;
}
```

$ disass trythis
Dump of assembler code for function trythis:
   0x00000000004011e6 <+0>:     push   %rbp
   0x00000000004011e7 <+1>:     mov    %rsp,%rbp
   0x00000000004011ea <+4>:     sub    $0x20,%rsp
   0x00000000004011ee <+8>:     call   0x401060 <mcount@plt>
   0x00000000004011f3 <+13>:    mov    %rdi,-0x18(%rbp)
   0x00000000004011f7 <+17>:    movq   $0x989680,-0x8(%rbp)
   0x00000000004011ff <+25>:    mov    $0x21,%edx
   0x0000000000401204 <+30>:    mov    $0x402058,%esi
   0x0000000000401209 <+35>:    mov    $0x402008,%edi
   0x000000000040120e <+40>:    mov    $0x0,%eax
   0x0000000000401213 <+45>:    call   0x401040 <printf@plt>
   0x0000000000401218 <+50>:    jmp    0x401222 <trythis+60>
   0x000000000040121a <+52>:    lock addl $0x1,0x2e4e(%rip)        # 0x404070 <counter>
   0x0000000000401222 <+60>:    mov    -0x8(%rbp),%rax
   0x0000000000401226 <+64>:    lea    -0x1(%rax),%rdx
   0x000000000040122a <+68>:    mov    %rdx,-0x8(%rbp)
   0x000000000040122e <+72>:    test   %rax,%rax
   0x0000000000401231 <+75>:    jne    0x40121a <trythis+52>
   0x0000000000401233 <+77>:    mov    $0x0,%eax
   0x0000000000401238 <+82>:    leave
   0x0000000000401239 <+83>:    ret
End of assembler dump.
