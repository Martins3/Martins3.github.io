#include <iostream>
#include <mutex>
#include <thread>
#include <atomic>
//
// # 通过反汇编理解 cpp 的 memory model
// <!-- 9c6aee81-6004-4347-aa77-57043df788dc -->
//

std::atomic<int> a;
int b;
int c;
void test_load()
{
	a.load(std::memory_order_relaxed);
	a.load(std::memory_order_consume);
	a.load(std::memory_order_acquire);
	a.load(std::memory_order_release);
	a.load(std::memory_order_acq_rel);
	a.load(std::memory_order_seq_cst);
	/*
	 * ldr     wzr, [x8]
   	 * ldapr   wzr, [x8]
   	 * ldapr   wzr, [x8]
   	 * ldr     wzr, [x8]
   	 * ldr     wzr, [x8]
   	 * ldar    wzr, [x8]
	 */

	/*
	 * lea    0x2e25(%rip),%rax        # 0x401c <a>
   	 * mov    (%rax),%ecx
   	 * mov    (%rax),%ecx
   	 * mov    (%rax),%ecx
   	 * mov    (%rax),%ecx
   	 * mov    (%rax),%ecx
   	 * mov    (%rax),%eax
   	 * ret
	 * */
}

void test_store()
{
	a.store(1, std::memory_order_relaxed);
	a.store(1, std::memory_order_consume);
	a.store(1, std::memory_order_acquire);
	a.store(1, std::memory_order_release);
	a.store(1, std::memory_order_acq_rel);
	a.store(1, std::memory_order_seq_cst);
	a++;
	/*
	 * str     w8, [x9]
   	 * str     w8, [x9]
   	 * str     w8, [x9]
   	 * stlr    w8, [x9]
   	 * str     w8, [x9]
   	 * stlr    w8, [x9]
	 */

	/*
	 * lea    0x2e05(%rip),%rax        # 0x401c <a>
         * movl   $0x1,(%rax)
         * movl   $0x1,(%rax)
         * movl   $0x1,(%rax)
         * movl   $0x1,(%rax)
         * movl   $0x1,(%rax)
	 *
         * mov    $0x1,%ecx
         * xchg   %ecx,(%rax)
	 *
         * lock incl (%rax)
         * ret
	 */
}

void test_opt()
{
	b = 1;
	b = 1;
	b = 1;
	b = 1;
	b = 1;
	b = 1;
	/*
	 * 如果一般的操作，那么将会直接被优化了:
	 *
	 * adrp    x8, 0x440000 <_ZSt9terminatev@got.plt>
   	 * mov     w9, #0x1                        // #1
   	 * str     w9, [x8, #104]
   	 * ret
	 * */
}

void test_inc()
{
	a++;

	a.fetch_add(1, std::memory_order_relaxed);
	a.fetch_add(1, std::memory_order_consume);
	a.fetch_add(1, std::memory_order_acquire);
	a.fetch_add(1, std::memory_order_release);
	a.fetch_add(1, std::memory_order_acq_rel);
	a.fetch_add(1, std::memory_order_seq_cst);

	/*
	 *
	 * ldaddal w8, w10, [x9]
	 *
         * ldadd   w8, w10, [x9]
         * ldadda  w8, w10, [x9]
         * ldadda  w8, w10, [x9]
         * ldaddl  w8, w10, [x9]
         * ldaddal w8, w10, [x9]
         * ldaddal w8, w8, [x9]
	 *
	 * 默认采用 ldaddal ，也就是 acquire and release general registers 其实
	 * 完全没有必要了。
	 *
	 * memory_order_acq_rel 其实是可以不用 memory_order_acq_rel 的
	 */

	/*
	 * lea    0x2dc5(%rip),%rax        # 0x401c <a>
   	 * lock incl (%rax)
   	 * lock incl (%rax)
   	 * lock incl (%rax)
   	 * lock incl (%rax)
   	 * lock incl (%rax)
   	 * lock incl (%rax)
   	 * lock incl (%rax)
   	 * ret
	 * */
}

void test_branch()
{
	/*
	 * 各种 fence ， 是用来清空 buffer 的，所以，
	 * 即便是 if else 是不会携带任何的 fence 的语义在其中的，
	 *
	 * 如果在另外一个 thread 中，先执行了 a fence b ，但是
	 * 因为 if 没有 fence 的， 还是可以 b 拿到新的数值，
	 * 但是 a 没有拿到新的数值。
	 *
	 * 所以，这里有三种情况:
	 * 1. 两个指令没有依赖关系
	 * 2. 两个指令有依赖关系，例如这里的 if else
	 * 3. 两个指令有 fence 保证的 store 关系
	 *
	 * 不要错误的理解第二种关系，指令的提交本来就是有顺序的，
	 * 没有 if else ，也会让两个指令循序执行的，
	 * 但是这个就是 load buffer 和 store buffer 的诡异地方，他的提交可以不用到 cache 中，
	 * 因为 cache 太慢了。
	 *
	 * 一下的反汇编结果完全符合预期:
	 *
	 * 如果是 : std::memory_order_relaxed
	 *
	 * adrp    x8, 0x440000 <_ZSt9terminatev@got.plt>
   	 * ldr     w8, [x8, #100]
   	 * cbz     w8, 0x4102c8 <_Z11test_branchv+16>
   	 * ret
   	 * adrp    x8, 0x440000 <_ZSt9terminatev@got.plt>
   	 * ldr     w9, [x8, #104]
   	 * add     w9, w9, #0x1
   	 * str     w9, [x8, #104]
   	 * ret
	 *
	 * 如果是 : std::memory_order_seq_cst
	 *
	 * adrp    x8, 0x440000 <_ZSt9terminatev@got.plt>
   	 * add     x8, x8, #0x64
   	 * ldar    w8, [x8]
   	 * cbz     w8, 0x4102cc <_Z11test_branchv+20>
   	 * ret
   	 * adrp    x8, 0x440000 <_ZSt9terminatev@got.plt>
   	 * ldr     w9, [x8, #104]
   	 * add     w9, w9, #0x1
   	 * str     w9, [x8, #104]
   	 * ret
	 * */
	if (!a.load(std::memory_order_seq_cst)) {
		++b;
	}
}

void test_cmp_exchange()
{
	int cmp = 1;
	int target = 1;
	/*
	 * 真的有趣，如果是 aarch64 的确是可以测试出来 cas casl casa casal 的
	 *
	 * 但是如果是 x86 ，一条 cmpxchg 指令全部搞定:
	 * lea    0x2d85(%rip),%rcx        # 0x401c <a>
   	 * mov    $0x1,%edx
   	 * mov    $0x1,%eax
   	 * lock cmpxchg %edx,(%rcx)
   	 * lock cmpxchg %edx,(%rcx)
   	 * lock cmpxchg %edx,(%rcx)
   	 * lock cmpxchg %edx,(%rcx)
   	 * lock cmpxchg %edx,(%rcx)
   	 * ret
	 */
	std::atomic_compare_exchange_weak_explicit(&a, &cmp, target,
						   std::memory_order_release,
						   std::memory_order_relaxed);

	std::atomic_compare_exchange_weak_explicit(&a, &cmp, target,
						   std::memory_order_relaxed,
						   std::memory_order_relaxed);

	std::atomic_compare_exchange_weak_explicit(&a, &cmp, target,
						   std::memory_order_relaxed,
						   std::memory_order_release);

	std::atomic_compare_exchange_weak_explicit(&a, &cmp, target,
						   std::memory_order_seq_cst,
						   std::memory_order_seq_cst);

	std::atomic_compare_exchange_weak_explicit(&a, &cmp, target,
						   std::memory_order_acquire,
						   std::memory_order_acquire);
}

void test_swap()
{
	/*
	 * arm 中的 swp 是原子指令的，开销很大，普通的 swap 显然不会如此。
	 * */
	std::swap(b, c);
}

std::binary_semaphore smph{ 0 };
void test_swap2()
{
	/*
	 * 这个会调用到库中，也看不到 swp 指令的具体使用
	 */
	smph.release();
	smph.acquire();
}

int main(int argc, char *argv[])
{
	std::thread st(test_load);
	std::thread st2(test_store);
	std::thread st3(test_opt);
	return 0;
}
