// Linux/x86 AVX2 STREAM-style bandwidth. Counting rules: see README.md.
#include <algorithm>
#include <barrier>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <immintrin.h>
#include <map>
#include <sched.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;
constexpr size_t MiB = 1024 * 1024;
constexpr size_t Huge = 2 * MiB;

namespace {
[[noreturn]] void fail(const std::string &message)
{
	throw std::runtime_error(message);
}

std::string read_text(const std::string &path)
{
	std::ifstream f(path);
	std::string s;
	if (!std::getline(f, s))
		fail("cannot read " + path);
	return s;
}

std::vector<std::string> split(const std::string &s)
{
	std::vector<std::string> result;
	std::istringstream in(s);
	std::string item;
	while (std::getline(in, item, ',')) {
		if (item.empty())
			fail("empty list item");
		result.push_back(item);
	}
	if (result.empty() || s.back() == ',')
		fail("empty list item");
	return result;
}

int number(const std::string &s, int minimum, int maximum)
{
	size_t end = 0;
	int n = std::stoi(s, &end);
	if (end != s.size() || n < minimum || n > maximum)
		fail("number out of range: " + s);
	return n;
}

std::vector<int> numbers(const std::string &s, int minimum, int maximum)
{
	std::vector<int> out;
	for (const auto &item : split(s)) {
		int n = number(item, minimum, maximum);
		if (std::find(out.begin(), out.end(), n) != out.end())
			fail("duplicate list item: " + item);
		out.push_back(n);
	}
	return out;
}

// One allowed thread per physical core first, then remaining SMT siblings.
// Explicit CPU lists are recorded; this is not a portable P/E classifier.
std::vector<int> cpu_order()
{
	cpu_set_t allowed;
	if (sched_getaffinity(0, sizeof allowed, &allowed))
		fail("sched_getaffinity failed");
	std::map<std::pair<int, int>, std::vector<int>> cores;
	for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
		if (!CPU_ISSET(cpu, &allowed))
			continue;
		auto dir = "/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/topology/";
		cores[{std::stoi(read_text(dir + "physical_package_id")),
		       std::stoi(read_text(dir + "core_id"))}].push_back(cpu);
	}
	std::vector<int> cpus;
	for (const auto &[key, siblings] : cores) {
		(void)key;
		cpus.push_back(siblings.front());
	}
	for (const auto &[key, siblings] : cores) {
		(void)key;
		cpus.insert(cpus.end(), siblings.begin() + 1, siblings.end());
	}
	if (cpus.empty())
		fail("empty CPU affinity");
	return cpus;
}

void pin(int cpu)
{
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	if (sched_setaffinity(0, sizeof mask, &mask)) {
		std::fprintf(stderr, "cannot pin CPU %d: %s\n", cpu, std::strerror(errno));
		std::exit(1); // Cannot unwind one worker while others wait at a barrier.
	}
}

std::string cpu_list(const std::vector<int> &cpus)
{
	std::string s;
	for (int cpu : cpus)
		s += (s.empty() ? "" : "+") + std::to_string(cpu);
	return s;
}

struct Mapping {
	double *data;
	size_t bytes;
	void *reservation;
	size_t reserved;
	Mapping(size_t size, const std::string &mode) : bytes(size)
	{
		int flags = MAP_PRIVATE | MAP_ANONYMOUS;
		if (mode == "hugetlb") {
			flags |= MAP_HUGETLB | (21 << MAP_HUGE_SHIFT);
			void *p = mmap(nullptr, size, PROT_READ | PROT_WRITE, flags, -1, 0);
			if (p == MAP_FAILED)
				fail("2 MiB hugetlb mmap: " + std::string(std::strerror(errno)) +
				     " (requires a sufficient hugepage pool and permission)");
			data = static_cast<double *>(p);
			reservation = p;
			reserved = size;
		} else {
			// Guard both sides against VMA merging with adjacent thread stacks.
			reserved = size + 2 * Huge;
			void *p = mmap(nullptr, reserved, PROT_NONE, flags, -1, 0);
			if (p == MAP_FAILED)
				fail("mmap: " + std::string(std::strerror(errno)));
			auto raw = reinterpret_cast<uintptr_t>(p);
			auto aligned = (raw + Huge) & ~(Huge - 1);
			reservation = p;
			data = reinterpret_cast<double *>(aligned);
			if (mprotect(data, size, PROT_READ | PROT_WRITE) ||
			    madvise(data, size, mode == "4k" ? MADV_NOHUGEPAGE : MADV_HUGEPAGE)) {
				const std::string error = std::strerror(errno);
				munmap(reservation, reserved);
				fail("madvise: " + error);
			}
		}
	}
	~Mapping() { munmap(reservation, reserved); }
	Mapping(const Mapping &) = delete;
	Mapping &operator=(const Mapping &) = delete;

	size_t huge_kib() const
	{
		std::ifstream in("/proc/self/smaps");
		std::string line;
		size_t total = 0, coverage = 0;
		bool ours = false;
		auto lo = reinterpret_cast<uintptr_t>(data);
		while (std::getline(in, line)) {
			unsigned long start, end;
			if (std::sscanf(line.c_str(), "%lx-%lx", &start, &end) == 2) {
				ours = start >= lo && end <= lo + bytes;
				if (ours)
					coverage += end - start;
			} else if (ours) {
				size_t kb;
				if (std::sscanf(line.c_str(), "AnonHugePages: %zu kB", &kb) == 1 ||
				    std::sscanf(line.c_str(), "Private_Hugetlb: %zu kB", &kb) == 1 ||
				    std::sscanf(line.c_str(), "Shared_Hugetlb: %zu kB", &kb) == 1)
					total += kb;
			}
		}
		if (coverage != bytes)
			fail("cannot verify exact mapping coverage in smaps: " + std::to_string(coverage) + "/" + std::to_string(bytes));
		return total;
	}
};

enum class Op { Read, Write, Copy, Triad, WriteNT, CopyNT, TriadNT };
struct Kernel { const char *name; Op op; int streams; };
const std::vector<Kernel> kernels = {
	{"read", Op::Read, 1}, {"write", Op::Write, 1}, {"copy", Op::Copy, 2},
	{"triad", Op::Triad, 3}, {"write_nt", Op::WriteNT, 1},
	{"copy_nt", Op::CopyNT, 2}, {"triad_nt", Op::TriadNT, 3},
};

// Slices are multiples of 16 doubles: no SIMD tail, misalignment, or false
// sharing, including non-power-of-two thread counts such as 24.
size_t boundary(size_t count, size_t t, size_t nt)
{
	return (count / 16 * t / nt) * 16;
}

__attribute__((noinline)) double read_avx(const double *a, size_t lo, size_t hi)
{
	__m256d s0 = _mm256_setzero_pd(), s1 = s0, s2 = s0, s3 = s0;
	for (size_t i = lo; i < hi; i += 16) {
		s0 = _mm256_add_pd(s0, _mm256_load_pd(a + i));
		s1 = _mm256_add_pd(s1, _mm256_load_pd(a + i + 4));
		s2 = _mm256_add_pd(s2, _mm256_load_pd(a + i + 8));
		s3 = _mm256_add_pd(s3, _mm256_load_pd(a + i + 12));
	}
	alignas(32) double sums[4];
	_mm256_store_pd(sums, _mm256_add_pd(_mm256_add_pd(s0, s1), _mm256_add_pd(s2, s3)));
	return sums[0] + sums[1] + sums[2] + sums[3];
}

template<Op op>
__attribute__((noinline)) double kernel(double *a, double *b, const double *c,
				       size_t lo, size_t hi)
{
	if constexpr (op == Op::Read) {
		return read_avx(a, lo, hi);
	} else {
		const __m256d five = _mm256_set1_pd(5.0), three = _mm256_set1_pd(3.0);
		for (size_t i = lo; i < hi; i += 4) {
			__m256d v;
			if constexpr (op == Op::Write || op == Op::WriteNT)
				v = five;
			else if constexpr (op == Op::Copy || op == Op::CopyNT)
				v = _mm256_load_pd(a + i);
			else
				v = _mm256_fmadd_pd(three, _mm256_load_pd(c + i), _mm256_load_pd(b + i));
			double *dst = (op == Op::Copy || op == Op::CopyNT) ? b + i : a + i;
			if constexpr (op == Op::WriteNT || op == Op::CopyNT || op == Op::TriadNT)
				_mm256_stream_pd(dst, v);
			else
				_mm256_store_pd(dst, v);
		}
		// Drain stores before completion. Cached stores can leave dirty LLC
		// lines; this fence does NOT measure persistence to DRAM.
		_mm_sfence();
		return 0;
	}
}

using Body = double (*)(double *, double *, const double *, size_t, size_t);
Body body(Op op)
{
	switch (op) {
	case Op::Read: return kernel<Op::Read>;
	case Op::Write: return kernel<Op::Write>;
	case Op::Copy: return kernel<Op::Copy>;
	case Op::Triad: return kernel<Op::Triad>;
	case Op::WriteNT: return kernel<Op::WriteNT>;
	case Op::CopyNT: return kernel<Op::CopyNT>;
	case Op::TriadNT: return kernel<Op::TriadNT>;
	}
	fail("invalid operation");
}

struct Options {
	size_t bytes = 512 * MiB;
	int samples = 7, passes = 4;
	std::vector<std::string> pages = {"4k", "thp"};
	std::vector<int> cpus = cpu_order(), threads;
	std::vector<Kernel> ops = kernels;
};

Options options(int argc, char **argv)
{
	Options o;
	const auto allowed = o.cpus;
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--help") {
			std::puts("stream.out [--pages 4k,thp,hugetlb] [--mib 512] [--samples 7]\n"
				  "           [--passes 4] [--cpus 0,2,4,6] [--threads 1,2,4]\n"
				  "           [--ops read,write,copy,triad,write_nt,copy_nt,triad_nt]\n"
				  "stdout: per-sample CSV; stderr: pages, CPUs, medians.\n"
				  "--mib is per array, a multiple of 2; three arrays are allocated.\n"
				  "HugeTLB failure is fatal; no fallback or automatic pool changes.");
			std::exit(0);
		}
		if (++i == argc)
			fail("missing value for " + arg);
		std::string value = argv[i];
		if (arg == "--mib")
			o.bytes = size_t(number(value, 2, 1048576)) * MiB;
		else if (arg == "--samples")
			o.samples = number(value, 1, 10000);
		else if (arg == "--passes")
			o.passes = number(value, 1, 10000);
		else if (arg == "--pages")
			o.pages = split(value);
		else if (arg == "--cpus")
			o.cpus = numbers(value, 0, CPU_SETSIZE - 1);
		else if (arg == "--threads")
			o.threads = numbers(value, 1, CPU_SETSIZE);
		else if (arg == "--ops") {
			o.ops.clear();
			for (const auto &name : split(value)) {
				auto it = std::find_if(kernels.begin(), kernels.end(),
					[&](const Kernel &k) { return name == k.name; });
				if (it == kernels.end())
					fail("unknown operation: " + name);
				o.ops.push_back(*it);
			}
		} else
			fail("unknown argument: " + arg);
	}
	if (o.bytes % Huge)
		fail("--mib must be a multiple of 2");
	for (const auto &mode : o.pages)
		if (mode != "4k" && mode != "thp" && mode != "hugetlb")
			fail("unknown page mode: " + mode);
	for (int cpu : o.cpus)
		if (std::find(allowed.begin(), allowed.end(), cpu) == allowed.end())
			fail("CPU not in allowed affinity: " + std::to_string(cpu));
	if (o.threads.empty()) {
		for (int n : {1, 2, 4, 8, 16, 24, 32})
			if (n <= int(o.cpus.size()))
				o.threads.push_back(n);
		if (o.threads.back() != int(o.cpus.size()))
			o.threads.push_back(int(o.cpus.size()));
	}
	for (int n : o.threads)
		if (n > int(o.cpus.size()))
			fail("thread count exceeds selected CPUs");
	return o;
}

void run_op(Mapping &mapping, const Options &o, const std::string &mode,
	    const std::vector<int> &cpus, const Kernel &k)
{
	const size_t count = o.bytes / sizeof(double), nt = cpus.size();
	double *a = mapping.data, *b = a + count, *c = b + count;
	std::vector<std::thread> workers;
	// Parallel first touch/reset before timing, using the same CPU placement.
	for (size_t t = 0; t < nt; ++t) {
		workers.emplace_back([&, t] {
			pin(cpus[t]);
			size_t lo = boundary(count, t, nt), hi = boundary(count, t + 1, nt);
			std::fill(a + lo, a + hi, 1.0);
			std::fill(b + lo, b + hi, 2.0);
			std::fill(c + lo, c + hi, 3.0);
		});
	}
	for (auto &th : workers)
		th.join();
	workers.clear();
#ifdef MADV_COLLAPSE
	if (mode == "thp" && madvise(mapping.data, mapping.bytes, MADV_COLLAPSE))
		std::fprintf(stderr, "MADV_COLLAPSE: %s (reporting actual coverage)\n", std::strerror(errno));
#endif
	const size_t huge_before = mapping.huge_kib();
	Clock::time_point t0, t1;
	std::barrier start(static_cast<std::ptrdiff_t>(nt + 1), [&]() noexcept { t0 = Clock::now(); });
	std::barrier finish(static_cast<std::ptrdiff_t>(nt + 1), [&]() noexcept { t1 = Clock::now(); });
	std::vector<double> sums(nt), times;
	const auto fn = body(k.op);
	for (size_t t = 0; t < nt; ++t) {
		workers.emplace_back([&, t] {
			pin(cpus[t]);
			const size_t lo = boundary(count, t, nt), hi = boundary(count, t + 1, nt);
			for (int sample = -1; sample < o.samples; ++sample) {
				start.arrive_and_wait();
				double sum = 0;
				for (int pass = 0; pass < o.passes; ++pass) {
					// Keep every sweep observable even under future LTO builds.
					asm volatile("" ::: "memory");
					sum += fn(a, b, c, lo, hi);
				}
				sums[t] = sum;
				finish.arrive_and_wait();
			}
		});
	}
	for (int sample = -1; sample < o.samples; ++sample) {
		start.arrive_and_wait();
		finish.arrive_and_wait();
		if (sample >= 0)
			times.push_back(std::chrono::duration<double, std::nano>(t1 - t0).count());
	}
	for (auto &th : workers)
		th.join();
	const size_t huge_after = mapping.huge_kib();
	if (mode == "4k" && (huge_before || huge_after))
		fail("unexpected huge pages in 4k mode");
	// Validate ALL elements outside timing, before emitting any result rows.
	double expected_a = 1, expected_b = 2;
	if (k.op == Op::Write || k.op == Op::WriteNT)
		expected_a = 5;
	if (k.op == Op::Copy || k.op == Op::CopyNT)
		expected_b = 1;
	if (k.op == Op::Triad || k.op == Op::TriadNT)
		expected_a = 11;
	for (size_t i = 0; i < count; ++i)
		if (a[i] != expected_a || b[i] != expected_b || c[i] != 3)
			fail("array validation failed for " + std::string(k.name));
	if (k.op == Op::Read) {
		for (size_t t = 0; t < nt; ++t) {
			size_t lo = boundary(count, t, nt), hi = boundary(count, t + 1, nt);
			if (sums[t] != double(hi - lo) * o.passes)
				fail("read checksum failed");
		}
	}
	const double payload = double(o.bytes) * k.streams * o.passes;
	std::vector<double> rates;
	for (int s = 0; s < o.samples; ++s) {
		double rate = payload / times[s];
		rates.push_back(rate);
		std::printf("%s,%s,%s,%zu,%zu,%d,%d,%.0f,%.0f,%.6f,%zu,%zu\n",
			    mode.c_str(), k.name, cpu_list(cpus).c_str(), nt, o.bytes,
			    o.passes, s, payload, times[s], rate, huge_before, huge_after);
	}
	std::sort(rates.begin(), rates.end());
	double median = (rates[(rates.size() - 1) / 2] + rates[rates.size() / 2]) / 2;
	std::fprintf(stderr, "%s %-8s %2zu threads %4zu MiB median %6.2f GB/s [%6.2f..%6.2f] huge=%zu/%zu KiB\n",
		     mode.c_str(), k.name, nt, o.bytes / MiB, median, rates.front(), rates.back(),
		     huge_after, mapping.bytes / 1024);
	std::fflush(stdout);
	if (std::ferror(stdout))
		fail("CSV write failed");
}
} // namespace

int main(int argc, char **argv)
{
	try {
		auto o = options(argc, argv);
		std::fprintf(stderr, "CPU order: %s\narray=%zu MiB samples=%d passes=%d; validation enabled\n",
			     cpu_list(o.cpus).c_str(), o.bytes / MiB, o.samples, o.passes);
		std::puts("pages,op,cpus,threads,array_bytes,passes,sample,payload_bytes,elapsed_ns,gb_s,huge_kib_before,huge_kib_after");
		for (const auto &mode : o.pages) {
			Mapping mapping(3 * o.bytes, mode);
			for (int n : o.threads) {
				std::vector<int> cpus(o.cpus.begin(), o.cpus.begin() + n);
				for (const auto &k : o.ops)
					run_op(mapping, o, mode, cpus, k);
			}
		}
	} catch (const std::exception &e) {
		std::fprintf(stderr, "stream: %s\n", e.what());
		return 1;
	}
}
