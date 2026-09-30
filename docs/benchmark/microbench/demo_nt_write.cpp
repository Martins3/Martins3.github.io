// Demo: does a write allocate in the cache?  See README "demo_nt_write".
//
// A: streaming write over 256 MiB (cold destination): cached store vs the
//    three non-temporal stores (vmovntps / vmovntpd / vmovntdq) at every
//    available width. NT is write-around (no RFO), so on a cold destination
//    it should move about twice the payload bandwidth; the three mnemonics
//    must match at equal width (they differ in type only).
// B: memcpy / memset size sweeps on a cold destination, plus hand-rolled
//    cached and NT copies at 256 MiB. glibc switches to a non-temporal loop
//    past __x86_shared_non_temporal_threshold / __x86_memset_non_temporal_
//    threshold (see __memcpy_avx_unaligned_erms); the sweep hunts that step.
// C: write a random 64 B-node ring with cached or NT stores, chase it at
//    once: ns/load is L2/L3 if the write stayed cached, DRAM if it did not.
//
// Widths: vmovnt{ps,pd,dq} differ in type, not in memory semantics. The
// 512-bit kernels are compiled under target("avx512f") and only run where
// the CPU has it; on AVX2-only machines they are reported as skipped.
//
// TODO 这个本来想要测试 simd 指令不会经过 cache ，结果他这个搞的太复杂了
#include <immintrin.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <sched.h>
#include <string>
#include <sys/mman.h>
#include <vector>

using Clock = std::chrono::steady_clock;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

namespace
{
u64 sink = 0;

[[noreturn]] void fail(const std::string &what)
{
	throw std::runtime_error(what);
}

template <class T> inline void keep(T &value)
{
	asm volatile("" : "+r"(value) : : "memory");
}

double elapsed(Clock::time_point begin)
{
	return std::chrono::duration<double, std::nano>(Clock::now() - begin)
		.count();
}

void pin(int cpu)
{
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(cpu, &set);
	if (sched_setaffinity(0, sizeof(set), &set))
		fail("sched_setaffinity");
}

struct Buffer {
	void *data = nullptr;
	u64 size = 0;
	explicit Buffer(u64 bytes) : size(bytes)
	{
		data = mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
			    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (data == MAP_FAILED)
			fail("mmap");
	}
	~Buffer()
	{
		if (data)
			munmap(data, size);
	}
	Buffer(const Buffer &) = delete;
	Buffer &operator=(const Buffer &) = delete;
};

double median(std::vector<double> v)
{
	std::sort(v.begin(), v.end());
	return v[v.size() / 2];
}

// Invalidate every line of a buffer. A big streaming read does not reliably
// evict recently used data (RRIP keeps it), so the "cold" side of every
// measurement here is clflush, not a scan.
void flush(const void *p, u64 bytes)
{
	const auto *q = static_cast<const char *>(p);
	for (u64 i = 0; i < bytes; i += 64)
		_mm_clflush(q + i);
	_mm_mfence();
}

// --- A: streaming writes ---------------------------------------------------
// Each variant stores 32 B / 64 B / 16 B per instruction over the whole
// buffer, reps times. sfence is inside the timed region so NT stores cannot
// sit in the WC buffers past the stop time.

__attribute__((noinline)) u64 w_cached_ymm(void *dst, u64 bytes, u64 reps)
{
	auto *p = static_cast<float *>(dst);
	const u64 n = bytes / 32;
	for (u64 r = 0; r < reps; ++r) {
		const __m256 v = _mm256_set1_ps((float)(r + 1));
		for (u64 i = 0; i < n; ++i)
			_mm256_store_ps(p + i * 8, v);
		asm volatile("" : : : "memory");
	}
	_mm_sfence();
	return bytes * reps; // payload bytes
}

__attribute__((noinline)) u64 w_nt_ps_ymm(void *dst, u64 bytes, u64 reps)
{
	auto *p = static_cast<float *>(dst);
	const u64 n = bytes / 32;
	for (u64 r = 0; r < reps; ++r) {
		const __m256 v = _mm256_set1_ps((float)(r + 1));
		for (u64 i = 0; i < n; ++i)
			_mm256_stream_ps(p + i * 8, v);
		asm volatile("" : : : "memory");
	}
	_mm_sfence();
	return bytes * reps; // payload bytes
}

__attribute__((noinline)) u64 w_nt_pd_ymm(void *dst, u64 bytes, u64 reps)
{
	auto *p = static_cast<double *>(dst);
	const u64 n = bytes / 32;
	for (u64 r = 0; r < reps; ++r) {
		const __m256d v = _mm256_set1_pd((double)(r + 1));
		for (u64 i = 0; i < n; ++i)
			_mm256_stream_pd(p + i * 4, v);
		asm volatile("" : : : "memory");
	}
	_mm_sfence();
	return bytes * reps; // payload bytes
}

__attribute__((noinline)) u64 w_nt_dq_ymm(void *dst, u64 bytes, u64 reps)
{
	auto *p = static_cast<__m256i *>(dst);
	const u64 n = bytes / 32;
	for (u64 r = 0; r < reps; ++r) {
		const __m256i v = _mm256_set1_epi32((int)(r + 1));
		for (u64 i = 0; i < n; ++i)
			_mm256_stream_si256(p + i, v);
		asm volatile("" : : : "memory");
	}
	_mm_sfence();
	return bytes * reps; // payload bytes
}

__attribute__((noinline)) u64 w_nt_dq_xmm(void *dst, u64 bytes, u64 reps)
{
	auto *p = static_cast<__m128i *>(dst);
	const u64 n = bytes / 16;
	for (u64 r = 0; r < reps; ++r) {
		const __m128i v = _mm_set1_epi32((int)(r + 1));
		for (u64 i = 0; i < n; ++i)
			_mm_stream_si128(p + i, v);
		asm volatile("" : : : "memory");
	}
	_mm_sfence();
	return bytes * reps; // payload bytes
}

__attribute__((noinline, target("avx512f"))) u64
w_cached_zmm(void *dst, u64 bytes, u64 reps)
{
	auto *p = static_cast<float *>(dst);
	const u64 n = bytes / 64;
	for (u64 r = 0; r < reps; ++r) {
		const __m512 v = _mm512_set1_ps((float)(r + 1));
		for (u64 i = 0; i < n; ++i)
			_mm512_store_ps(p + i * 16, v);
		asm volatile("" : : : "memory");
	}
	_mm_sfence();
	return bytes * reps; // payload bytes
}

__attribute__((noinline, target("avx512f"))) u64
w_nt_ps_zmm(void *dst, u64 bytes, u64 reps)
{
	auto *p = static_cast<float *>(dst);
	const u64 n = bytes / 64;
	for (u64 r = 0; r < reps; ++r) {
		const __m512 v = _mm512_set1_ps((float)(r + 1));
		for (u64 i = 0; i < n; ++i)
			_mm512_stream_ps(p + i * 16, v);
		asm volatile("" : : : "memory");
	}
	_mm_sfence();
	return bytes * reps; // payload bytes
}

__attribute__((noinline, target("avx512f"))) u64
w_nt_pd_zmm(void *dst, u64 bytes, u64 reps)
{
	auto *p = static_cast<double *>(dst);
	const u64 n = bytes / 64;
	for (u64 r = 0; r < reps; ++r) {
		const __m512d v = _mm512_set1_pd((double)(r + 1));
		for (u64 i = 0; i < n; ++i)
			_mm512_stream_pd(p + i * 8, v);
		asm volatile("" : : : "memory");
	}
	_mm_sfence();
	return bytes * reps; // payload bytes
}

__attribute__((noinline, target("avx512f"))) u64
w_nt_dq_zmm(void *dst, u64 bytes, u64 reps)
{
	auto *p = static_cast<__m512i *>(dst);
	const u64 n = bytes / 64;
	for (u64 r = 0; r < reps; ++r) {
		const __m512i v = _mm512_set1_epi32((int)(r + 1));
		for (u64 i = 0; i < n; ++i)
			_mm512_stream_si512(p + i, v);
		asm volatile("" : : : "memory");
	}
	_mm_sfence();
	return bytes * reps; // payload bytes
}

struct WriteVariant {
	const char *name;
	u64 (*fn)(void *, u64, u64);
	bool avx512;
};

// --- B: copies -------------------------------------------------------------
// Hand-rolled copies for the reference ceilings; memcpy/memset themselves are
// called per timed sample so the size argument selects one glibc path.

__attribute__((noinline)) u64
copy_cached_ymm(void *dst, const void *src, u64 bytes, u64 reps)
{
	const auto *s = static_cast<const float *>(src);
	auto *d = static_cast<float *>(dst);
	const u64 n = bytes / 32;
	for (u64 r = 0; r < reps; ++r) {
		for (u64 i = 0; i < n; ++i)
			_mm256_store_ps(d + i * 8, _mm256_load_ps(s + i * 8));
		asm volatile("" : : : "memory");
	}
	_mm_sfence();
	return bytes * reps; // payload bytes
}

__attribute__((noinline)) u64
copy_nt_ymm(void *dst, const void *src, u64 bytes, u64 reps)
{
	const auto *s = static_cast<const float *>(src);
	auto *d = static_cast<float *>(dst);
	const u64 n = bytes / 32;
	for (u64 r = 0; r < reps; ++r) {
		for (u64 i = 0; i < n; ++i)
			_mm256_stream_ps(d + i * 8, _mm256_load_ps(s + i * 8));
		asm volatile("" : : : "memory");
	}
	_mm_sfence();
	return bytes * reps; // payload bytes
}

// --- C: write a random ring, then chase it ---------------------------------
struct alignas(64) Node {
	int next;
	char pad[60];
};
static_assert(sizeof(Node) == 64);

// Sequential stores of ring data (nxt is a permutation successor), so the
// write side is the same streaming pattern as A and B.
__attribute__((noinline)) void ring_cached(Node *nodes, const int *nxt, u64 n)
{
	for (u64 i = 0; i < n; ++i) {
		_mm256_store_si256((__m256i *)&nodes[i],
				   _mm256_set_epi32(0, 0, 0, 0, 0, 0, 0,
						    nxt[i]));
		_mm256_store_si256((__m256i *)&nodes[i] + 1,
				   _mm256_setzero_si256());
	}
	_mm_sfence();
}

__attribute__((noinline)) void ring_nt(Node *nodes, const int *nxt, u64 n)
{
	for (u64 i = 0; i < n; ++i) {
		_mm256_stream_si256((__m256i *)&nodes[i],
				    _mm256_set_epi32(0, 0, 0, 0, 0, 0, 0,
						     nxt[i]));
		_mm256_stream_si256((__m256i *)&nodes[i] + 1,
				    _mm256_setzero_si256());
	}
	_mm_sfence();
}

__attribute__((noinline)) u64 chase(const Node *nodes, u64 steps)
{
	int p = 0;
	for (u64 k = 0; k < steps; ++k)
		p = nodes[p].next;
	keep(p);
	return (u32)p;
}

// --- glibc string thresholds -----------------------------------------------
// Local BSS symbols; resolve the offset from libc.so.6's symtab and the load
// base from /proc/self/maps. Failures just print n/a.
u64 libc_base()
{
	std::ifstream in("/proc/self/maps");
	std::string line;
	while (std::getline(in, line)) {
		if (line.find("libc.so.6") == std::string::npos)
			continue;
		unsigned long long start = 0, end = 0, off = 0;
		char perms[8];
		if (std::sscanf(line.c_str(), "%llx-%llx %7s %llx", &start, &end,
				perms, &off) == 4)
			return u64(start - off);
	}
	return 0;
}

void print_glibc_thresholds()
{
	std::map<std::string, u64> offset;
	if (FILE *nm = popen("nm /usr/lib64/libc.so.6 2>/dev/null", "r")) {
		char line[256], name[128];
		unsigned long long addr;
		char type;
		while (std::fgets(line, sizeof line, nm))
			if (std::sscanf(line, "%llx %c %127s", &addr, &type,
					name) == 3)
				offset[name] = addr;
		pclose(nm);
	}
	const u64 base = libc_base();
	const char *names[] = { "__x86_shared_non_temporal_threshold",
				"__x86_memset_non_temporal_threshold",
				"__x86_rep_movsb_threshold" };
	std::cout << "glibc 2.42 string thresholds (bytes):\n";
	for (const char *name : names) {
		std::cout << "  " << std::left << std::setw(40) << name;
		auto it = offset.find(name);
		if (!base || it == offset.end()) {
			std::cout << "n/a\n";
			continue;
		}
		std::cout << *(const u64 *)(base + it->second) << '\n';
	}
	std::cout << std::right;
}

// --- experiments -----------------------------------------------------------
void run_writes(const Buffer &buf, bool avx512)
{
	const u64 bytes = 256ull * 1024 * 1024;
	const u64 reps = 2;
	const int samples = 5;
	const WriteVariant variants[] = {
		{ "cached  vmovaps  ymm 32B", w_cached_ymm, false },
		{ "nt      vmovntps ymm 32B", w_nt_ps_ymm, false },
		{ "nt      vmovntpd ymm 32B", w_nt_pd_ymm, false },
		{ "nt      vmovntdq ymm 32B", w_nt_dq_ymm, false },
		{ "nt      vmovntdq xmm 16B", w_nt_dq_xmm, false },
		{ "cached  vmovaps  zmm 64B", w_cached_zmm, true },
		{ "nt      vmovntps zmm 64B", w_nt_ps_zmm, true },
		{ "nt      vmovntpd zmm 64B", w_nt_pd_zmm, true },
		{ "nt      vmovntdq zmm 64B", w_nt_dq_zmm, true },
	};
	std::cout << "\n== A: streaming write, " << bytes / (1024 * 1024)
		  << " MiB cold destination, reps=" << reps
		  << " ==\npayload GB/s (median of " << samples << ")\n";
	for (const auto &v : variants) {
		if (v.avx512 && !avx512) {
			std::cout << "  " << v.name << "    skipped (no avx512f)\n";
			continue;
		}
		std::vector<double> gbs;
		for (int s = 0; s < samples; ++s) {
			flush(buf.data, bytes);
			const u64 ops = v.fn(buf.data, bytes, reps);
			const auto start = Clock::now();
			v.fn(buf.data, bytes, reps);
			gbs.push_back(double(ops) / elapsed(start));
			sink ^= ops;
		}
		std::cout << "  " << v.name << "    " << std::fixed
			  << std::setprecision(1) << median(gbs) << '\n';
	}
	std::cout << std::defaultfloat;
}

void sweep(const Buffer &src, const Buffer &dst, const char *name,
	   bool is_memset, u64 max_bytes, int samples)
{
	std::cout << "\n== B: " << name
		  << ", cold destination, single call per sample ==\n"
		  << "payload GB/s (median of " << samples << ")\n";
	const u64 sizes[] = { 4 << 10,  16 << 10, 64 << 10, 256 << 10,
			      1 << 20,  2 << 20,  4 << 20,  6 << 20,
			      7 << 20,  8 << 20,  9 << 20,  10 << 20,
			      12 << 20, 16 << 20, 24 << 20, 32 << 20,
			      48 << 20, 64 << 20, 96 << 20, 128 << 20,
			      192 << 20, 256 << 20 };
	for (u64 bytes : sizes) {
		if (bytes > max_bytes)
			continue;
		std::vector<double> gbs;
		for (int s = 0; s < samples; ++s) {
			flush(dst.data, bytes);
			if (!is_memset)
				flush(src.data, bytes);
			const auto start = Clock::now();
			if (is_memset)
				std::memset(dst.data, 0x5a, bytes);
			else
				std::memcpy(dst.data, src.data, bytes);
			gbs.push_back(double(bytes) / elapsed(start));
		}
		std::cout << "  " << std::setw(7) << bytes / 1024 << " KiB    "
			  << std::fixed << std::setprecision(1) << median(gbs)
			  << '\n';
	}
	std::cout << std::defaultfloat;
}

void compare_copies(const Buffer &src, const Buffer &dst)
{
	const u64 bytes = 256ull * 1024 * 1024;
	const u64 reps = 2;
	const int samples = 3;
	std::cout << "\n== B2: copy reference at " << bytes / (1024 * 1024)
		  << " MiB cold ==\npayload GB/s (median of " << samples
		  << ")\n";
	const struct {
		const char *name;
		u64 (*fn)(void *, const void *, u64, u64);
		bool glibc;
	} variants[] = {
		{ "hand   cached  ymm", copy_cached_ymm, false },
		{ "hand   nt      ymm", copy_nt_ymm, false },
	};
	for (const auto &v : variants) {
		std::vector<double> gbs;
		for (int s = 0; s < samples; ++s) {
			flush(dst.data, bytes);
			flush(src.data, bytes);
			const u64 ops = v.fn(dst.data, src.data, bytes, reps);
			const auto start = Clock::now();
			v.fn(dst.data, src.data, bytes, reps);
			gbs.push_back(double(ops) / elapsed(start));
			sink ^= ops;
		}
		std::cout << "  " << v.name << "    " << std::fixed
			  << std::setprecision(1) << median(gbs) << '\n';
	}
	for (const char *name : { "memcpy", "memmove", "memset" }) {
		std::vector<double> gbs;
		for (int s = 0; s < samples; ++s) {
			flush(dst.data, bytes);
			flush(src.data, bytes);
			const auto start = Clock::now();
			for (u64 r = 0; r < reps; ++r) {
				if (!std::strcmp(name, "memcpy"))
					std::memcpy(dst.data, src.data, bytes);
				else if (!std::strcmp(name, "memmove"))
					std::memmove(dst.data, src.data, bytes);
				else
					std::memset(dst.data, 0x5a, bytes);
				asm volatile("" : : : "memory");
			}
			gbs.push_back(double(bytes * reps) / elapsed(start));
		}
		std::cout << "  glibc  " << std::left << std::setw(8) << name
			  << std::right << "       " << std::fixed
			  << std::setprecision(1) << median(gbs) << '\n';
	}
	std::cout << std::defaultfloat;
}

void run_residency()
{
	const u64 sizes[] = { 256ull << 10, 2ull << 20, 8ull << 20 };
	const int samples = 9;
	std::cout << "\n== C: first-pass chase right after writing the ring ==\n"
		  << "ns/load over exactly one ring cycle (n dependent first "
		     "visits), median of "
		  << samples << "; L2 4 MiB, L3 16 MiB on this box\n";
	for (u64 bytes : sizes) {
		Buffer nodes_buf(bytes);
		auto *nodes = static_cast<Node *>(nodes_buf.data);
		const u64 n = bytes / sizeof(Node);
		std::vector<int> order(n), nxt(n);
		std::iota(order.begin(), order.end(), 0);
		std::mt19937 rng(42);
		std::shuffle(order.begin(), order.end(), rng);
		for (u64 i = 0; i < n; ++i)
			nxt[order[i]] = order[(i + 1) % n];
		for (const char *path : { "cached", "nt   " }) {
			std::vector<double> hot, cold;
			for (int s = 0; s < samples; ++s) {
				flush(nodes, bytes);
				if (!std::strcmp(path, "cached"))
					ring_cached(nodes, nxt.data(), n);
				else
					ring_nt(nodes, nxt.data(), n);
				// One ring cycle timed right after the write:
				// every node is a first visit, so the latency
				// is whatever the write left behind. A second
				// cycle would see the chase's own footprint.
				{
					const auto start = Clock::now();
					sink ^= chase(nodes, n);
					hot.push_back(elapsed(start) /
						      double(n));
				}
				flush(nodes, bytes);
				{
					const auto start = Clock::now();
					sink ^= chase(nodes, n);
					cold.push_back(elapsed(start) /
						       double(n));
				}
			}
			std::cout << "  " << std::setw(6) << bytes / 1024
				  << " KiB write=" << path << "    after write "
				  << std::fixed << std::setprecision(1)
				  << median(hot) << " ns    after clflush "
				  << median(cold) << " ns\n";
		}
	}
	std::cout << std::defaultfloat;
}
} // namespace

int main()
{
	try {
		pin(2);
		const bool avx512 = __builtin_cpu_supports("avx512f");
		std::cout << "demo_nt_write  avx512f=" << (avx512 ? "yes" : "no")
			  << '\n';
		print_glibc_thresholds();

		const u64 buf_bytes = 256ull * 1024 * 1024;
		Buffer src(buf_bytes), dst(buf_bytes);
		std::memset(src.data, 0x11, src.size);

		run_writes(dst, avx512);
		sweep(src, dst, "memcpy", false, buf_bytes, 7);
		sweep(src, dst, "memset", true, buf_bytes, 7);
		compare_copies(src, dst);
		run_residency();
		std::cout << "\nsink " << sink << '\n';
	} catch (const std::exception &e) {
		std::cerr << "fail: " << e.what() << '\n';
		return 1;
	}
	return 0;
}
