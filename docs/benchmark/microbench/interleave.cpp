// Linux/x86: map individual cache lines to the two ADL/RPL IMCs.
// Counter units and interpretation (especially DDR5): see README.md.
#include <algorithm>
#include <array>
#include <bit>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cpuid.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <immintrin.h>
#include <linux/perf_event.h>
#include <numeric>
#include <map>
#include <set>
#include <unordered_map>
#include <random>
#include <sched.h>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace
{
using Clock = std::chrono::steady_clock;
constexpr size_t Page = 4096, Line = 64;


[[noreturn]] void fail(const std::string &s)
{
	throw std::runtime_error(s);
}

std::string read_text(const std::string &path)
{
	std::ifstream f(path);
	std::string s;
	if (!std::getline(f, s))
		fail("cannot read " + path);
	return s;
}

int number(const std::string &s, int lo, int hi)
{
	size_t end;
	int n = std::stoi(s, &end);
	if (end != s.size() || n < lo || n > hi)
		fail("out of range: " + s);
	return n;
}

struct Fd {
	int fd;
	explicit Fd(int n)
		: fd(n)
	{
	}
	~Fd()
	{
		if (fd >= 0)
			close(fd);
	}
	Fd(const Fd &) = delete;
	Fd &operator=(const Fd &) = delete;
};

struct Counter {
	Fd handle;
	struct Reading {
		uint64_t count, enabled, running;
	};

	static int open_counter(int imc, bool reads)
	{
		const auto path =
			"/sys/bus/event_source/devices/uncore_imc_free_running_" +
			std::to_string(imc) + "/";
		const std::string event = reads ? "data_read" : "data_write";
		// Deliberately restrict this tool to a verified PMU layout.
		if (read_text(path + "events/" + event) !=
			    (reads ? "event=0xff,umask=0x20" :
				     "event=0xff,umask=0x30") ||
		    read_text(path + "format/event") != "config:0-7" ||
		    read_text(path + "format/umask") != "config:8-15" ||
		    read_text(path + "events/" + event + ".unit") != "MiB" ||
		    std::stod(read_text(path + "events/" + event + ".scale")) !=
			    64.0 / 1048576)
			fail("unsupported PMU encoding or counter unit: " +
			     path);
		perf_event_attr attr{};
		attr.size = sizeof attr;
		attr.type = std::stoul(read_text(path + "type"));
		attr.config = reads ? 0x20ff : 0x30ff;
		attr.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED |
				   PERF_FORMAT_TOTAL_TIME_RUNNING;
		// cpumask is the PMU's representative CPU, not the benchmark CPU.
		int cpu = std::stoi(read_text(path + "cpumask"));
		int fd = syscall(SYS_perf_event_open, &attr, -1, cpu, -1,
				 PERF_FLAG_FD_CLOEXEC);
		if (fd < 0)
			fail("perf_event_open " + path + ": " +
			     std::strerror(errno) +
			     " (run with sudo; system-wide IMC access is required)");
		return fd;
	}
	Counter(int imc, bool reads)
		: handle(open_counter(imc, reads))
	{
	}
	Reading read() const
	{
		Reading r{};
		if (::read(handle.fd, &r, sizeof r) != sizeof r)
			fail("short perf counter read");
		return r;
	}
};

struct Window {
	double ns;
	std::array<double, 2> counts;
};

template <class Work>
Window measure(const Counter &c0, const Counter &c1, Work work)
{
	auto begin = Clock::now();
	std::array<Counter::Reading, 2> a{ c0.read(), c1.read() };
	work();
	std::array<Counter::Reading, 2> b{ c0.read(), c1.read() };
	Window w{ std::chrono::duration<double, std::nano>(Clock::now() - begin)
			  .count(),
		  {} };
	for (int i = 0; i < 2; ++i) {
		uint64_t enabled = b[i].enabled - a[i].enabled;
		uint64_t running = b[i].running - a[i].running;
		if (!running || running != enabled)
			fail("IMC counter stopped or multiplexed");
		w.counts[i] = static_cast<double>(b[i].count - a[i].count);
	}
	return w;
}

// Four aligned 16-byte NT stores fill exactly one 64-byte WC buffer.
// Fence every iteration so consecutive iterations cannot merge in that buffer.
// MFENCE also orders the final verification load. This measures routing, not bandwidth.
__attribute__((noinline)) void write_line(char *p, int iterations)
{
	for (int i = 0; i < iterations; ++i) {
		__m128i value = _mm_set1_epi32(i);
		for (int j = 0; j < 4; ++j)
			_mm_stream_si128(reinterpret_cast<__m128i *>(p) + j,
					 value);
		_mm_mfence();
	}
}

// Repeatedly evict and reload the same line. LFENCE completes the load before
// the next CLFLUSH. No sequential stream; adjacent lines stay cached after warm-up.
__attribute__((noinline)) void read_line(char *p, int iterations)
{
	for (int i = 0; i < iterations; ++i) {
		_mm_clflush(p);
		_mm_mfence();
		(void)*reinterpret_cast<volatile uint64_t *>(p);
		_mm_lfence();
	}
}

struct Mapping {
	char *data;
	size_t bytes;
	Fd pagemap{ ::open("/proc/self/pagemap", O_RDONLY | O_CLOEXEC) };
	explicit Mapping(size_t size)
		: bytes(size)
	{
		if (pagemap.fd < 0)
			fail("cannot open pagemap");
		data = static_cast<char *>(
			mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
			     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
		if (data == MAP_FAILED)
			fail("mmap failed");
		if (madvise(data, bytes, MADV_NOHUGEPAGE) ||
		    mlock(data, bytes)) {
			munmap(data, bytes);
			fail("MADV_NOHUGEPAGE/mlock failed");
		}
		std::memset(
			data, 0,
			bytes); // Real private pages, not shared zero pages.
		for (size_t i = 0; i < bytes; i += Line)
			_mm_clflush(data + i);
		_mm_mfence();
	}
	~Mapping()
	{
		munmap(data, bytes);
	}
	Mapping(const Mapping &) = delete;
	Mapping &operator=(const Mapping &) = delete;
	uint64_t physical(size_t offset) const
	{
		uint64_t entry;
		off_t pos =
			(reinterpret_cast<uintptr_t>(data + offset) / Page) *
			sizeof entry;
		if (pread(pagemap.fd, &entry, sizeof entry, pos) !=
			    sizeof entry ||
		    !(entry & (1ULL << 63)) || (entry & (1ULL << 62)))
			fail("page not resident");
		uint64_t pfn = entry & ((1ULL << 55) - 1);
		if (!pfn)
			fail("pagemap PFN hidden: CAP_SYS_ADMIN is required");
		return pfn * Page + offset % Page;
	}
};

// Use a differential signal: balanced background traffic cancels even if its
// total rate changes. IMC write merging can reduce requests below iterations.
// Reject weak/oversized signals and unstable background imbalance. Preserve
// negative residuals; clipping would bias the differential measurement.
int label(double a, double b, double drift, int iterations)
{
	double n = iterations;
	double difference = a - b;
	if (drift > 0.15 * n)
		return -1;
	if (difference >= 0.5 * n && difference <= 1.25 * n)
		return 0;
	if (difference <= -0.5 * n && difference >= -1.25 * n)
		return 1;
	return -1;
}

struct Pair {
	size_t a, b; // Virtual offsets into the pool, NOT assumed physically contiguous.
	unsigned bit;
};

// For high bits find actual physical XOR partners, rather than adding a virtual
// stride. Randomize selection instead of using only the first neighboring pages.
std::vector<Pair> make_pairs(const std::vector<uint64_t> &physical, int count)
{
	std::unordered_map<uint64_t, size_t> by_pa;
	for (size_t p = 0; p < physical.size(); ++p)
		if (!by_pa.emplace(physical[p], p).second)
			fail("duplicate physical page");
	std::vector<size_t> order(physical.size());
	std::iota(order.begin(), order.end(), 0);
	std::mt19937 rng(0x12864);
	unsigned bits = std::bit_width(*std::max_element(physical.begin(), physical.end()) + Page - 1);
	std::vector<Pair> pairs;
	for (unsigned bit = 6; bit < bits; ++bit) {
		std::shuffle(order.begin(), order.end(), rng);
		int found = 0;
		for (size_t p : order) {
			size_t offset = (rng() % 64) * Line;
			if (bit < 12) {
				offset &= ~size_t(1ULL << bit);
				pairs.push_back({p * Page + offset, p * Page + (offset ^ (1ULL << bit)), bit});
			} else {
				uint64_t other = physical[p] ^ (1ULL << bit);
				auto it = by_pa.find(other);
				if (physical[p] > other || it == by_pa.end()) continue;
				pairs.push_back({p * Page + offset, it->second * Page + offset, bit});
			}
			if (++found == count) break;
		}
	}
	return pairs;
}

int vote(const std::array<int, 3> &v, int samples)
{
	return v[1] * 3 >= samples * 2 && !v[2] ? 0 :
	       v[2] * 3 >= samples * 2 && !v[1] ? 1 : -1;
}

// An effect is accepted only with sufficient pairs and unanimous bit effects.
int bit_effect(int planned, int valid, int flips)
{
	if (valid < 4 || valid * 3 < planned * 2) return -1;
	if (flips == 0) return 0;
	if (flips == valid) return 1;
	return -1;
}

void self_test()
{
	// Shuffled, non-contiguous virtual-to-physical mapping, plus a physical hole.
	std::vector<uint64_t> physical;
	for (uint64_t p = 0; p < 256; ++p)
		if (p != 17) physical.push_back((p + 1024) * Page);
	std::mt19937 rng(42);
	std::shuffle(physical.begin(), physical.end(), rng);
	auto pairs = make_pairs(physical, 16);
	std::array<int, 64> coverage{};
	std::set<std::pair<unsigned, uint64_t>> unique;
	for (const auto &pair : pairs) {
		uint64_t a = physical.at(pair.a / Page) + pair.a % Page;
		uint64_t b = physical.at(pair.b / Page) + pair.b % Page;
		if ((a ^ b) != (1ULL << pair.bit) || pair.a % Line || pair.b % Line ||
		    !unique.emplace(pair.bit, std::min(a, b)).second) fail("physical XOR pair test failed");
		++coverage[pair.bit];
		for (uint64_t mask : {64ULL, 128ULL, 0x20600ULL}) {
			int ca = std::popcount(a & mask) & 1;
			int cb = std::popcount(b & mask) & 1;
			if ((ca ^ cb) != int((mask >> pair.bit) & 1)) fail("XOR effect test failed");
		}
	}
	for (int bit = 6; bit < 20; ++bit)
		if (coverage[bit] != 16) fail("cross-page coverage test failed");
	if (coverage[20] || coverage[21] || coverage[22]) fail("invented absent partners");
	if (vote({0, 5, 0}, 5) != 0 || vote({1, 0, 4}, 5) != 1 ||
	    vote({0, 4, 1}, 5) != -1 || vote({3, 2, 0}, 5) != -1 ||
	    bit_effect(16, 16, 0) != 0 || bit_effect(16, 16, 16) != 1 ||
	    bit_effect(16, 16, 15) != -1 || bit_effect(16, 3, 3) != -1 ||
	    label(1000, 0, 0, 1000) != 0 || label(0, 1000, 0, 1000) != 1 ||
	    label(500, 500, 0, 1000) != -1 || label(1000, 0, 300, 1000) != -1 ||
	    label(2000, 0, 0, 1000) != -1 || label(1000, -400, 0, 1000) != -1 ||
	    label(100000, 101000, 0, 1000) != 1) fail("classification test failed");
	std::puts("self-test: physical XOR pairs, cross-page coverage, missing partners, 64B/128B/hash and noise rejection PASS");
}

void check_machine()
{
	unsigned a, b, c, d;
	__cpuid(0, a, b, c, d);
	if (b != 0x756e6547 || d != 0x49656e69 || c != 0x6c65746e)
		fail("Intel CPU required");
	__cpuid(1, a, b, c, d);
	unsigned model = ((a >> 4) & 15) | ((a >> 12) & 0xf0);
	if (((a >> 8) & 15) != 6 ||
	    (model != 0x97 && model != 0x9a && model != 0xb7 && model != 0xba &&
	     model != 0xbf))
		fail("only Alder/Raptor Lake supported");
	if (c & (1U << 31))
		fail("run on bare metal; guest addresses/PMUs cannot prove host routing");
	if (!(d & (1U << 19)) || ((b >> 8) & 255) * 8 != Line ||
	    sysconf(_SC_PAGESIZE) != Page)
		fail("64B CLFLUSH lines and 4KiB base pages required");
	std::fprintf(stderr,
		     "Intel family=6 model=0x%x; IMC0/IMC1 request counters\n",
		     model);
}

int run(int argc, char **argv)
{
	int cpu = -1, mib = 256, pair_count = 32, samples = 5, iterations = 20000;
	bool reads = true;
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--self-test") { self_test(); return 0; }
		if (arg == "--help") {
			std::puts("sudo ./interleave.out [--mib 256] [--pairs 32] [--cpu N]\n"
				  "                     [--samples 5] [--iterations 20000] [--access read|write]\n"
				  "Samples physical XOR pairs across the pool; prints a summary only, writes no files.\n"
				  "ADL/RPL bare metal; IMCs, not DDR5 subchannels. Exit 2: inconclusive.\n"
				  "--self-test needs no root.");
			return 0;
		}
		if (i + 1 == argc) fail("missing value for " + arg);
		std::string value = argv[++i];
		if (arg == "--cpu") cpu = number(value, 0, CPU_SETSIZE - 1);
		else if (arg == "--mib") mib = number(value, 1, 4096);
		else if (arg == "--pairs") pair_count = number(value, 4, 256);
		else if (arg == "--samples") samples = number(value, 3, 31);
		else if (arg == "--iterations") iterations = number(value, 1000, 10000000);
		else if (arg == "--access" && (value == "read" || value == "write")) reads = value == "read";
		else fail("unknown option " + arg);
	}
	if (!(samples & 1)) fail("samples must be odd");
	check_machine();
	cpu_set_t allowed, pinned;
	if (sched_getaffinity(0, sizeof allowed, &allowed)) fail("cannot read affinity");
	if (cpu < 0)
		for (int i = 0; i < CPU_SETSIZE; ++i)
			if (CPU_ISSET(i, &allowed)) { cpu = i; break; }
	if (cpu < 0 || !CPU_ISSET(cpu, &allowed)) fail("CPU outside allowed affinity");
	CPU_ZERO(&pinned);
	CPU_SET(cpu, &pinned);
	if (sched_setaffinity(0, sizeof pinned, &pinned)) fail("cannot pin CPU");
	Counter c0(0, reads), c1(1, reads);
	Mapping memory(size_t(mib) * 1024 * 1024);
	std::vector<uint64_t> physical(memory.bytes / Page);
	for (size_t p = 0; p < physical.size(); ++p) physical[p] = memory.physical(p * Page);
	auto pairs = make_pairs(physical, pair_count);
	std::map<size_t, size_t> point_index;
	for (const auto &pair : pairs)
		for (size_t pos : {pair.a, pair.b})
			if (!point_index.count(pos)) point_index.emplace(pos, point_index.size());
	std::vector<size_t> points(point_index.size());
	std::set<uint64_t> tested_pages;
	for (auto [pos, index] : point_index) {
		points[index] = pos;
		tested_pages.insert(physical[pos / Page]);
	}
	if (tested_pages.empty()) fail("no physical address pairs found");
	std::printf("Pool: %d MiB / %zu pages; sampled: %zu pages, %zu lines, %zu address pairs\n",
		mib, physical.size(), tested_pages.size(), points.size(), pairs.size());
	std::printf("Sampled physical page bases: 0x%llx .. 0x%llx (sparse, not exhaustive)\n",
		static_cast<unsigned long long>(*tested_pages.begin()),
		static_cast<unsigned long long>(*tested_pages.rbegin()));
	std::printf("access=%s cpu=%d samples=%d iterations=%d; running...\n",
		reads ? "read" : "write", cpu, samples, iterations);
	std::fflush(stdout);
	auto probe = reads ? read_line : write_line;
	std::vector<std::array<int, 3>> votes(points.size());
	std::vector<size_t> order(points.size());
	std::iota(order.begin(), order.end(), 0);
	std::mt19937 rng(0x12864);
	size_t rejected = 0;
	for (int sample = 0; sample < samples; ++sample) {
		std::shuffle(order.begin(), order.end(), rng);
		for (size_t index : order) {
			size_t pos = points[index];
			uint64_t expected = physical[pos / Page] + pos % Page;
			if (memory.physical(pos) != expected) fail("physical page migrated");
			char *ptr = memory.data + pos;
			probe(ptr, 1000);
			auto idle = [] { std::this_thread::sleep_for(std::chrono::milliseconds(5)); };
			Window before = measure(c0, c1, idle);
			Window active = measure(c0, c1, [&] { probe(ptr, iterations); });
			Window after = measure(c0, c1, idle);
			std::array<double, 2> net{};
			for (int j = 0; j < 2; ++j)
				net[j] = active.counts[j] - (before.counts[j] / before.ns +
					after.counts[j] / after.ns) * 0.5 * active.ns;
			double drift = std::abs((before.counts[0] - before.counts[1]) / before.ns -
				(after.counts[0] - after.counts[1]) / after.ns) * active.ns;
			if (memory.physical(pos) != expected) fail("physical page migrated");
			for (int j = 0; j < 16; ++j) {
				int value;
				std::memcpy(&value, ptr + j * sizeof value, sizeof value);
				if (value != (reads ? 0 : iterations - 1)) fail("data verification failed");
			}
			_mm_clflush(ptr);
			_mm_mfence();
			int result = label(net[0], net[1], drift, iterations);
			++votes[index][result + 1];
			rejected += result < 0;
		}
	}
	for (size_t pos : points)
		if (memory.physical(pos) != physical[pos / Page] + pos % Page) fail("physical page migrated");
	std::array<int, 64> planned{}, valid{}, flips{};
	for (const auto &pair : pairs) {
		++planned[pair.bit];
		int a = vote(votes[point_index.at(pair.a)], samples);
		int b = vote(votes[point_index.at(pair.b)], samples);
		if (a < 0 || b < 0) continue;
		++valid[pair.bit];
		flips[pair.bit] += a != b;
	}
	std::printf("Samples: %zu; rejected: %zu\n", points.size() * samples, rejected);
	std::puts("PA bit    distance(B)    valid/planned    channel flips    effect");
	unsigned bits = std::bit_width(*std::max_element(physical.begin(), physical.end()) + Page - 1);
	int lowest = -1;
	bool lower_known = true;
	for (unsigned bit = 6; bit < bits; ++bit) {
		int effect = bit_effect(planned[bit], valid[bit], flips[bit]);
		std::printf("%6u %14llu %8d/%-8d %10d       %s\n", bit,
			static_cast<unsigned long long>(1ULL << bit), valid[bit], planned[bit], flips[bit],
			!planned[bit] ? "not covered" : effect < 0 ? "inconclusive" : effect ? "flips" : "same");
		if (lowest < 0) {
			if (effect < 0) lower_known = false;
			if (effect == 1 && lower_known) lowest = bit;
		}
	}
	if (lowest < 0) {
		std::puts("INCONCLUSIVE: cannot establish the lowest IMC-select bit.");
		return 2;
	}
	std::printf("RESULT: lowest IMC-select bit = %d, granularity = %llu B\n", lowest,
		static_cast<unsigned long long>(1ULL << lowest));
	std::puts("Scope: sampled physical XOR pairs, including cross-page pairs; no full-address-space or complete-hash claim.\n"
		  "IMC selection is channel selection only on verified one-channel-per-IMC configurations.");
	return 0;
}

} // namespace

int main(int argc, char **argv)
{
	try {
		return run(argc, argv);
	} catch (const std::exception &e) {
		std::fprintf(stderr, "error: %s\n", e.what());
		return 1;
	}
}
