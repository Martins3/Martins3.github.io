#include "internal.h"

/*
 * put_cpu_var / put_cpu_ptr 只是相比 this_cpu_ptr 多了一个
 * preemption 的关闭而已
 */

struct foo {
	int head;
	int tail;
	/* Deferred action fifo queue storage. */
	int fifo[128];
};

#ifdef TODO_DEBUG_STATIC_PERCPU
static DEFINE_PER_CPU(int, name);
static DEFINE_PER_CPU(int *, name_ptr);
#endif
static struct foo __percpu *action_fifos;

/*
 * 如下测试可以发现:
 * 1. 装载的三个阶段
 *	- 变成 ko 后，虚拟地址变化
 *	- load 到 kernel ，虚拟地址再次变化，而且
 * 2. 的确用 gs 来实现 percpu 的功能的
 * 3. martins3.ko 显然 crash 是没有 debuginfo 的，但是有符号表，所以用 crash 还可以 disass
 *
 * 问题:
 * 1. 如何理解
 *  mov    %gs:0x3f778541(%rip),%rax        # 0x159e0
 * 2. percpu 的结构是什么?
 *    一个数据放到一起，还是按照 CPU 放到一起
 * 3. 为什么 cs jmp 指令最后翻译成了这个 ret
 *
 * 对于如下的代码，测试方法为:
 *
 static noinline int disass_percpu(void)
 {
 	action_fifos = alloc_percpu(struct foo);
 	if (!action_fifos)
 		return -ENOMEM;
 	struct foo *fifo = this_cpu_ptr(action_fifos);
 	return fifo->head == fifo->tail;
 }

 * 对于 percpu.o
Dump of assembler code for function disass:
   0x0000000000000050 <+0>:     call   0x55 <disass+5>
   0x0000000000000055 <+5>:     mov    $0x208,%edi
   0x000000000000005a <+10>:    mov    $0x4,%esi
   0x000000000000005f <+15>:    xor    %edx,%edx
   0x0000000000000061 <+17>:    mov    $0xcc0,%ecx
   0x0000000000000066 <+22>:    call   0x6b <disass+27>
   0x000000000000006b <+27>:    mov    %rax,0x0(%rip)        # 0x72 <disass+34>
   0x0000000000000072 <+34>:    test   %rax,%rax
   0x0000000000000075 <+37>:    je     0x7f <disass+47>
   0x0000000000000077 <+39>:    mov    %gs:0x0(%rip),%rax        # 0x7f <disass+47>
   0x000000000000007f <+47>:    cs jmp 0x85

# 对于 martins3.ko 的反汇编
   0x000000000000bf80 <+0>:     nopl   0x0(%rax,%rax,1)
   0x000000000000bf85 <+5>:     mov    $0x208,%edi
   0x000000000000bf8a <+10>:    mov    $0x4,%esi
   0x000000000000bf8f <+15>:    xor    %edx,%edx
   0x000000000000bf91 <+17>:    mov    $0xcc0,%ecx
   0x000000000000bf96 <+22>:    call   0xbf9b <disass+27>
   0x000000000000bf9b <+27>:    mov    %rax,0x0(%rip)        # 0xbfa2 <disass+34>
   0x000000000000bfa2 <+34>:    test   %rax,%rax
   0x000000000000bfa5 <+37>:    je     0xbfaf <disass+47>
   0x000000000000bfa7 <+39>:    mov    %gs:0x0(%rip),%rax        # 0xbfaf <disass+47>
   0x000000000000bfaf <+47>:    cs jmp 0xbfb5

# 将模块插入到 kernel 之后，使用 crash 来 disass

	0xffffffffc089d470 <disass>:    nopl   0x0(%rax,%rax,1) [FTRACE NOP]
	0xffffffffc089d475 <disass+5>:  mov    $0x208,%edi
	0xffffffffc089d47a <disass+10>: mov    $0x4,%esi
	0xffffffffc089d47f <disass+15>: xor    %edx,%edx
	0xffffffffc089d481 <disass+17>: mov    $0xcc0,%ecx
	0xffffffffc089d486 <disass+22>: call   0xffffffff81348980 <pcpu_alloc_noprof>
	0xffffffffc089d48b <disass+27>: mov    %rax,0xdd16(%rip)        # 0xffffffffc08ab1a8 <action_fifos>
	0xffffffffc089d492 <disass+34>: test   %rax,%rax
	0xffffffffc089d495 <disass+37>: je     0xffffffffc089d49f <disass+47>
	0xffffffffc089d497 <disass+39>: mov    %gs:0x3f778541(%rip),%rax        # 0x159e0
	0xffffffffc089d49f <disass+47>: ret
 */

/*
 * 考虑一下几个数值的计算:
 *
 * 0x000000000000c270 <+0>:     nopl   0x0(%rax,%rax,1)
 * 0x000000000000c275 <+5>:     mov    %gs:0x0(%rip),%rsi        # 0xc27d <disass_percpu+13>
 * 0x000000000000c27d <+13>:    add    0x0(%rip),%rsi        # 0xc284 <disass_percpu+20>
 * 0x000000000000c284 <+20>:    mov    $0x0,%rdi
 * 0x000000000000c28b <+27>:    jmp    0xc290 <__pfx_test_printk>
 *
 * crash> dis disass_percpu
 * 0xffffffffc08b76f0 <disass_percpu>:     nopl   0x0(%rax,%rax,1) [FTRACE NOP]
 * 0xffffffffc08b76f5 <disass_percpu+5>:   mov    %gs:0x3f75e2e3(%rip),%rsi        # 0x159e0
 * 0xffffffffc08b76fd <disass_percpu+13>:  add    0xdae4(%rip),%rsi        # 0xffffffffc08c51e8 <action_fifos>
 * 0xffffffffc08b7704 <disass_percpu+20>:  mov    $0xffffffffc07f0768,%rdi
 * 0xffffffffc08b770b <disass_percpu+27>:  jmp    0xffffffff821fba70 <_printk>
 *
 *
 * 逐个解释:
 * 1. mov    %gs:0x3f75e2e3(%rip),%rsi # %gs:0x3f75e2e3(%rip) 对应的内存 0xffff88807d040000 ，就是 gs 寄存器中存储的数值 ，编译的时候，不知道 this_cpu_off 的地址，所以填充为 0
 * 1. add    0xdae4(%rip),%rsi  # 从 0xdae4(%rip) 可以得到 607f82635a00 ，编译的时候静态变量的位置不确定，但是相对于 ip 的位置知道
 *
 * [  338.603801] CPU : 2
 * [  338.604310] action_fifos : 607f82635a00
 * [  338.604651] FS:  0000000000000000(0000) GS:ffff88807d040000(0000) knlGS:0000000000000000
 * [  338.604800] foo      : ffffe8ffff675a00
 *
 * TODO 这里的 percpu 实现还需要深究一下，其实两个指令明显可以优化为一个
 *
 */
#ifdef CONFIG_X86_64
static noinline int disass_percpu(void)
{
	// 思考下，this_cpu_ptr 和 this_cpu_read 的区别
	struct foo *fifo = this_cpu_ptr(action_fifos);
	pr_info("foo      : %px\n", fifo);

#ifdef TODO_DEBUG_STATIC_PERCPU
	// TODO 直接打印的是 0 ，此外，这有意义吗?
	int n = this_cpu_read(name);
	int *m = this_cpu_read(name_ptr);
	pr_info("name     : %d\n", n);
	pr_info("name_ptr : %p\n", m);
#endif

	// TODO 这个会触发错误
	// [  638.351818] BUG: unable to handle page fault for address: 0000000000032350
	/* pr_info("name         : %lx\n", (long)name); */
	/* pr_info("name_ptr     : %lx\n", (long)name_ptr); */

	// TODO 也会导致宕机
	// [    9.870850] BUG: unable to handle page fault for address: 0000000000032358
	/* int * k = this_cpu_ptr(name_ptr); */
	/* pr_info("name_ptr : %p\n", k); */

	// 看来，静态定义和动态定义的实现方法差别很大
	return fifo->head == fifo->tail;
}

static int show_percpu(void)
{
	unsigned int fsindex, gsindex;
	unsigned long fs, gs, shadowgs;
	// action_fifos 是正常的 pointer ，还是 percpu 的偏移?
	pr_info("action_fifos : %lx\n", (long)action_fifos);

	asm("movl %%fs,%0" : "=r"(fsindex));
	asm("movl %%gs,%0" : "=r"(gsindex));

	rdmsrl(MSR_FS_BASE, fs);
	rdmsrl(MSR_GS_BASE, gs);
	rdmsrl(MSR_KERNEL_GS_BASE, shadowgs);

	pr_info("FS:  %016lx(%04x) GS:%016lx(%04x) knlGS:%016lx\n", fs, fsindex,
		gs, gsindex, shadowgs);

	return disass_percpu();
}
#else
static int show_percpu(void)
{
	pr_info("[martins3:%s:%d] aarch64 not implemented yet\n", __FUNCTION__, __LINE__);
	return 0;
}
#endif

static void handler(void *data)
{
	pr_info("CPU : %d\n", smp_processor_id());
	show_percpu();
}

static void test(void)
{
	for (size_t i = 0; i < num_online_cpus(); i++)
		smp_call_function_single(i, handler, NULL, 1);
}

int test_percpu_init(void)
{
	action_fifos = alloc_percpu(struct foo);
	if (!action_fifos)
		return -ENOMEM;
	return 0;
}

int test_percpu_exit(void)
{
	free_percpu(action_fifos);
	return 0;
}

static int test2(void)
{
	struct foo __percpu *f;
	f = alloc_percpu(struct foo);
	if (!f)
		return -ENOMEM;
	pr_info("[martins3:%s:%d] %lx\n", __FUNCTION__, __LINE__, (long)f);
	free_percpu(action_fifos);
	pr_info("[martins3:%s:%d] %lx\n", __FUNCTION__, __LINE__, (long)f);
	struct foo *fifo = this_cpu_ptr(action_fifos);
	pr_info("fifo : %d\n", fifo->head == fifo->tail);
	return 0;
}


int test_percpu(long action)
{
	switch (action) {
	case 0:
		test();
		break;
	case 2:
		test2();
		break;
	}
	return 0;
}
