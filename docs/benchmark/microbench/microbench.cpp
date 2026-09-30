// Linux/x86-64 microbenchmarks. See README.md for measurement boundaries.
#include <algorithm>
#include <array>
#include <atomic>
#include <barrier>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <linux/perf_event.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

using Clock = std::chrono::steady_clock;
using u64 = std::uint64_t;
static_assert(std::atomic<u64>::is_always_lock_free);
static_assert(Clock::is_steady);

namespace
{
int samples = 5;
bool quick = false;
u64 sink = 0;

[[noreturn]] void fail(const std::string &what)
{
	throw std::runtime_error(what + ": " + std::strerror(errno));
}

template <class T> inline void keep(T &value)
{
	asm volatile("" : "+r"(value) : : "memory");
}

u64 count(u64 full)
{
	return quick ? std::max<u64>(1, full / 10) : full;
}

double elapsed(Clock::time_point begin)
{
	return std::chrono::duration<double, std::nano>(Clock::now() - begin)
		.count();
}

std::string read_text(const std::string &path)
{
	std::ifstream input(path);
	std::string text;
	std::getline(input, text);
	if (!input)
		throw std::runtime_error("cannot read " + path);
	return text;
}

void pin(int cpu)
{
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(cpu, &set);
	if (sched_setaffinity(0, sizeof(set), &set))
		fail("sched_setaffinity");
}

struct Core {
	std::vector<int> cpus;
	bool smt;
};

std::vector<Core> topology()
{
	cpu_set_t allowed;
	if (sched_getaffinity(0, sizeof(allowed), &allowed))
		fail("sched_getaffinity");
	std::map<std::pair<int, int>, Core> cores;
	for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
		if (!CPU_ISSET(cpu, &allowed))
			continue;
		const auto dir = "/sys/devices/system/cpu/cpu" +
				 std::to_string(cpu) + "/topology/";
		const int socket =
			std::stoi(read_text(dir + "physical_package_id"));
		const int id = std::stoi(read_text(dir + "core_id"));
		auto &core = cores[{ socket, id }];
		core.cpus.push_back(cpu);
		const auto siblings = read_text(dir + "thread_siblings_list");
		core.smt = siblings.find_first_of(",-") != std::string::npos;
	}
	std::vector<Core> result;
	for (auto &[key, core] : cores) {
		(void)key;
		result.push_back(core);
	}
	if (result.empty())
		throw std::runtime_error("empty CPU affinity");
	return result;
}

std::string cpu_list(const std::vector<int> &cpus)
{
	std::string text;
	for (int cpu : cpus) {
		if (!text.empty())
			text += '+'; // CSV-safe.
		text += std::to_string(cpu);
	}
	return text;
}

void row(const std::string &group, const std::string &name,
	 const std::string &cpus, u64 bytes, int sample, u64 operations,
	 double ns, const std::string &unit, double value)
{
	std::cout << group << ',' << name << ',' << cpus << ',' << bytes << ','
		  << sample << ',' << operations << ',' << ns << ',' << unit
		  << ',' << value << '\n';
	if (!std::cout)
		throw std::runtime_error("CSV write failed");
}

template <class F>
void measure(const std::string &group, const std::string &name, int cpu,
	     u64 bytes, u64 operations, const std::string &unit, F body)
{
	body();
	for (int s = 0; s < samples; ++s) {
		const auto start = Clock::now();
		body();
		const double ns = elapsed(start);
		const double value = unit == "GB/s" ? double(operations) / ns :
						      ns / double(operations);
		row(group, name, std::to_string(cpu), bytes, s, operations, ns,
		    unit, value);
	}
}

// One dependent integer multiply/add chain; unsigned wraparound is intentional.
__attribute__((noinline)) u64 chain(u64 n, u64 x)
{
	for (u64 i = 0; i < n; ++i) {
		x = x * 2862933555777941757ULL + 3037000493ULL;
		keep(x);
	}
	return x;
}

// Four independent scalar chains. Barriers prevent closed-form replacement/SIMD.
__attribute__((noinline)) u64 independent(u64 n, u64 seed)
{
	u64 a = seed, b = seed + 1, c = seed + 2, d = seed + 3;
	for (u64 i = 0; i < n; ++i) {
		a = a * 2862933555777941757ULL + 3037000493ULL;
		b = b * 2862933555777941757ULL + 3037000493ULL;
		c = c * 2862933555777941757ULL + 3037000493ULL;
		d = d * 2862933555777941757ULL + 3037000493ULL;
		asm volatile("" : "+r"(a), "+r"(b), "+r"(c), "+r"(d));
	}
	return a ^ b ^ c ^ d;
}

__attribute__((noinline)) u64
branch_loop(const std::vector<unsigned char> &bits, u64 rounds)
{
	u64 x = 0;
	for (u64 r = 0; r < rounds; ++r) {
		for (unsigned char bit : bits) {
			// Force an actual conditional branch, not compiler-generated cmov/SIMD.
			asm volatile("testb $1, %b1\n\tjz 1f\n\taddq $3, %0\n1:"
				     : "+r"(x)
				     : "q"(bit)
				     : "cc");
		}
	}
	return x;
}

void compute(int cpu)
{
	const u64 n = count(20000000);
	measure("compute", "dependent", cpu, 0, n, "ns/update",
		[&] { sink ^= chain(n, sink); });
	measure("compute", "independent4", cpu, 0, n * 4, "ns/update",
		[&] { sink ^= independent(n, sink); });
	std::vector<unsigned char> bits(65536);
	std::fill(bits.begin() + bits.size() / 2, bits.end(), 1);
	const u64 rounds = count(300);
	measure("branch", "sorted_50pct", cpu, bits.size(),
		rounds * bits.size(), "ns/branch",
		[&] { sink ^= branch_loop(bits, rounds); });
	std::mt19937 rng(42);
	std::shuffle(bits.begin(), bits.end(), rng);
	measure("branch", "random_50pct", cpu, bits.size(),
		rounds * bits.size(), "ns/branch",
		[&] { sink ^= branch_loop(bits, rounds); });
}

struct Mapping {
	void *data;
	size_t size;
	explicit Mapping(size_t bytes)
		: size(bytes)
	{
		data = mmap(nullptr, size, PROT_READ | PROT_WRITE,
			    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (data == MAP_FAILED)
			fail("mmap");
		if (madvise(data, size, MADV_NOHUGEPAGE)) {
			const int error = errno;
			munmap(data, size);
			errno = error;
			fail("MADV_NOHUGEPAGE");
		}
	}
	~Mapping()
	{
		munmap(data, size);
	}
	Mapping(const Mapping &) = delete;
	Mapping &operator=(const Mapping &) = delete;
};

struct alignas(64) Node {
	Node *next;
};
static_assert(sizeof(Node) == 64);

__attribute__((noinline)) Node *chase(Node *p, u64 steps)
{
	for (u64 i = 0; i < steps; ++i)
		p = p->next;
	keep(p);
	return p;
}

void memory(int cpu)
{
	for (size_t kib : { 4, 16, 32, 48, 64, 128, 512, 1024, 2048, 4096, 8192,
			    16384, 32768, 65536, 131072, 262144 }) {
		Mapping map(kib * 1024);
		const size_t nodes = map.size / sizeof(Node);
		std::vector<size_t> order(nodes);
		std::iota(order.begin(), order.end(), 0);
		std::mt19937 rng(42);
		std::shuffle(order.begin(), order.end(), rng);
		auto *data = static_cast<Node *>(map.data);
		for (size_t i = 0; i < nodes; ++i)
			new (&data[order[i]])
				Node{ &data[order[(i + 1) % nodes]] };
		Node *p = chase(
			data, nodes); // Fault in and traverse every cache line.
		const u64 steps = std::max<u64>(count(4000000), nodes * 2);
		measure("memory", "random_pointer_chase", cpu, map.size, steps,
			"ns/load", [&] { p = chase(p, steps); });
		sink ^= reinterpret_cast<std::uintptr_t>(p);
	}
	for (size_t kib : { 32, 1024, 16384, 262144 }) {
		Mapping src(kib * 1024), dst(kib * 1024);
		std::memset(src.data, 0x5a, src.size);
		std::memset(dst.data, 0, dst.size);
		const u64 rounds = std::max<u64>(
			2, count(512ULL * 1024 * 1024) / src.size);
		measure("memory", "memcpy_payload", cpu, src.size,
			rounds * src.size, "GB/s", [&] {
				for (u64 r = 0; r < rounds; ++r) {
					std::memcpy(dst.data, src.data,
						    src.size);
					asm volatile(""
						     :
						     : "r"(dst.data)
						     : "memory");
				}
			});
		if (std::memcmp(dst.data, src.data, src.size))
			throw std::runtime_error("memcpy mismatch");
		sink ^= static_cast<unsigned char *>(dst.data)[0];
	}
}

struct Usage {
	rusage value{};
	Usage()
	{
		if (getrusage(RUSAGE_THREAD, &value))
			fail("getrusage");
	}
	static double ns(timeval t)
	{
		return double(t.tv_sec) * 1e9 + double(t.tv_usec) * 1e3;
	}
};

// Per-thread PMU, kernel included. Permission failure means unavailable, never zero.
struct Counters {
	int cycles = -1, instructions = -1;
	Counters()
	{
		perf_event_attr attr{};
		attr.size = sizeof(attr);
		attr.type = PERF_TYPE_HARDWARE;
		attr.config = PERF_COUNT_HW_CPU_CYCLES;
		attr.disabled = 1;
		attr.exclude_hv = 1;
		attr.read_format = PERF_FORMAT_GROUP |
				   PERF_FORMAT_TOTAL_TIME_ENABLED |
				   PERF_FORMAT_TOTAL_TIME_RUNNING;
		cycles =
			static_cast<int>(syscall(SYS_perf_event_open, &attr, 0,
						 -1, -1, PERF_FLAG_FD_CLOEXEC));
		if (cycles < 0) {
			std::cerr << "PMU unavailable (kernel included): "
				  << std::strerror(errno) << '\n';
			return;
		}
		attr.config = PERF_COUNT_HW_INSTRUCTIONS;
		attr.disabled = 0;
		instructions = static_cast<int>(syscall(SYS_perf_event_open,
							&attr, 0, -1, cycles,
							PERF_FLAG_FD_CLOEXEC));
		if (instructions < 0) {
			std::cerr << "PMU instructions unavailable: "
				  << std::strerror(errno) << '\n';
			close(cycles);
			cycles = -1;
		}
	}
	~Counters()
	{
		if (instructions >= 0)
			close(instructions);
		if (cycles >= 0)
			close(cycles);
	}
	void start()
	{
		if (cycles < 0)
			return;
		if (ioctl(cycles, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP) ||
		    ioctl(cycles, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP))
			fail("PMU enable");
	}
	std::array<double, 2> stop()
	{
		if (cycles < 0)
			return { -1, -1 };
		if (ioctl(cycles, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP))
			fail("PMU disable");
		std::array<u64, 5> data{};
		if (read(cycles, data.data(), sizeof(data)) != sizeof(data))
			fail("PMU read");
		if (data[0] != 2 || !data[2])
			throw std::runtime_error("PMU did not run");
		// Only two events; reject multiplexing rather than mis-scale repeated resets.
		if (data[1] != data[2])
			throw std::runtime_error(
				"PMU multiplexed; rerun without competing profilers");
		return { double(data[3]), double(data[4]) };
	}
};

void syscalls(int cpu)
{
	const int fd = open("/dev/null", O_WRONLY | O_CLOEXEC);
	if (fd < 0)
		fail("/dev/null");
	const u64 n = count(1000000);
	Counters pmu;
	auto run = [&](const std::string &name, auto operation) {
		auto body = [&] {
			u64 total = 0;
			for (u64 i = 0; i < n; ++i) {
				total += operation(i);
				keep(total);
			}
			sink ^= total;
		};
		body();
		for (int s = 0; s < samples; ++s) {
			Usage before;
			pmu.start();
			const auto start = Clock::now();
			body();
			const double ns = elapsed(start);
			const auto counters = pmu.stop();
			Usage after;
			row("syscall", name, std::to_string(cpu), 0, s, n, ns,
			    "ns/call", ns / n);
			row("syscall_cpu", name, std::to_string(cpu), 0, s, n,
			    ns, "user_ns/call",
			    (Usage::ns(after.value.ru_utime) -
			     Usage::ns(before.value.ru_utime)) /
				    n);
			row("syscall_cpu", name, std::to_string(cpu), 0, s, n,
			    ns, "system_ns/call",
			    (Usage::ns(after.value.ru_stime) -
			     Usage::ns(before.value.ru_stime)) /
				    n);
			if (counters[0] >= 0) {
				row("syscall_pmu", name, std::to_string(cpu), 0,
				    s, n, ns, "cycles/call", counters[0] / n);
				row("syscall_pmu", name, std::to_string(cpu), 0,
				    s, n, ns, "instructions/call",
				    counters[1] / n);
			}
		}
	};
	run("loop_baseline", [](u64 i) { return i; });
	run("getpid_raw", [](u64) {
		const long result = syscall(SYS_getpid);
		if (result < 0)
			fail("getpid");
		return u64(result);
	});
	run("clock_vdso", [](u64) {
		timespec ts{};
		if (clock_gettime(CLOCK_MONOTONIC, &ts))
			fail("clock_gettime");
		return u64(ts.tv_nsec);
	});
	run("clock_raw", [](u64) {
		timespec ts{};
		if (syscall(SYS_clock_gettime, CLOCK_MONOTONIC, &ts))
			fail("clock_gettime syscall");
		return u64(ts.tv_nsec);
	});
	run("write_null", [&](u64) {
		if (write(fd, "x", 1) != 1)
			fail("write /dev/null");
		return u64(1);
	});
	close(fd);
}

std::vector<std::pair<std::string, std::vector<int> > >
placements(const std::vector<Core> &cores)
{
	std::vector<int> primary, single, all;
	for (const auto &core : cores) {
		if (core.smt) {
			primary.push_back(core.cpus[0]);
		} else
			single.push_back(core.cpus[0]);
		all.insert(all.end(), core.cpus.begin(), core.cpus.end());
	}
	std::vector<std::pair<std::string, std::vector<int> > > result;
	for (size_t n : { 1, 2, 4, 8 }) {
		if (primary.size() >= n)
			result.push_back(
				{ "smt_cores_" + std::to_string(n),
				  { primary.begin(), primary.begin() + n } });
		if (single.size() >= n)
			result.push_back(
				{ "single_thread_cores_" + std::to_string(n),
				  { single.begin(), single.begin() + n } });
	}
	std::vector<int> four_smt;
	for (const auto &core : cores) {
		if (core.cpus.size() < 2)
			continue;
		four_smt.insert(four_smt.end(), core.cpus.begin(),
				core.cpus.begin() + 2);
		if (four_smt.size() == 8)
			break;
	}
	if (four_smt.size() == 8)
		result.push_back({ "four_cores_eight_threads", four_smt });
	std::vector<int> physical;
	for (const auto &core : cores)
		physical.push_back(core.cpus[0]);
	result.push_back({ "all_physical_cores", physical });
	result.push_back({ "all_logical_cpus", all });
	return result;
}

void scaling(const std::vector<Core> &cores)
{
	auto configs = placements(cores);
	// Equal total work in every placement. Rotate order between samples.
	const u64 total = count(240000000);
	for (int s = -1; s < samples; ++s) {
		for (size_t k = 0; k < configs.size(); ++k) {
			const auto &[name, cpus] =
				configs[(k + size_t(s + 1)) % configs.size()];
			for (bool ilp : { false, true }) {
				std::barrier gate(static_cast<std::ptrdiff_t>(
					cpus.size() + 1));
				std::vector<std::thread> workers;
				std::vector<u64> values(cpus.size());
				for (size_t t = 0; t < cpus.size(); ++t)
					workers.emplace_back([&, t] {
						pin(cpus[t]);
						// Warm this CPU before timed work.
						values[t] = chain(
							count(1000000), t + 1);
						gate.arrive_and_wait(); // All workers ready before the clock starts.
						gate.arrive_and_wait();
						const u64 n =
							total / cpus.size() +
							(t <
							 total % cpus.size());
						values[t] =
							ilp ? independent(
								      n,
								      values[t]) :
							      chain(n,
								    values[t]);
						gate.arrive_and_wait();
					});
				gate.arrive_and_wait();
				const auto start = Clock::now();
				gate.arrive_and_wait();
				gate.arrive_and_wait();
				const double ns = elapsed(start);
				for (auto &worker : workers)
					worker.join();
				for (auto value : values)
					sink ^= value;
				if (s >= 0)
					row("scaling",
					    name + (ilp ? "_ilp4" : "_chain"),
					    cpu_list(cpus), 0, s,
					    total * (ilp ? 4 : 1), ns,
					    "Mupdates/s",
					    total * (ilp ? 4e3 : 1e3) / ns);
			}
		}
	}
}

std::vector<std::pair<std::string, std::vector<int> > >
pairs(const std::vector<Core> &cores, int cpu)
{
	const auto found =
		std::find_if(cores.begin(), cores.end(), [=](const Core &c) {
			return std::find(c.cpus.begin(), c.cpus.end(), cpu) !=
			       c.cpus.end();
		});
	const auto &base = *found;
	std::vector<std::pair<std::string, std::vector<int> > > result;
	for (int sibling : base.cpus)
		if (sibling != cpu) {
			result.push_back({ "smt_siblings", { cpu, sibling } });
			break;
		}
	bool same = false, other = false;
	for (const auto &core : cores) {
		if (&core == &base)
			continue;
		if (core.smt == base.smt && !same) {
			result.push_back({ "different_core_same_smt_type",
					   { cpu, core.cpus[0] } });
			same = true;
		} else if (core.smt != base.smt && !other) {
			result.push_back({ "different_core_other_smt_type",
					   { cpu, core.cpus[0] } });
			other = true;
		}
	}
	return result;
}

void coherence(const std::vector<Core> &cores, int cpu)
{
	const u64 n = count(1000000);
	for (const auto &[name, cpus] : pairs(cores, cpu)) {
		for (int s = -1; s < samples; ++s) {
			alignas(64) std::atomic<u64> turn{ 0 };
			std::barrier gate(2);
			std::thread peer([&] {
				pin(cpus[1]);
				gate.arrive_and_wait();
				for (u64 i = 0; i < n; ++i) {
					while (turn.load(
						       std::memory_order_acquire) !=
					       1)
						asm volatile("pause");
					turn.store(0,
						   std::memory_order_release);
				}
			});
			pin(cpus[0]);
			gate.arrive_and_wait();
			const auto start = Clock::now();
			for (u64 i = 0; i < n; ++i) {
				turn.store(1, std::memory_order_release);
				while (turn.load(std::memory_order_acquire) !=
				       0)
					asm volatile("pause");
			}
			const double ns = elapsed(start);
			peer.join();
			if (s >= 0)
				row("coherence", name, cpu_list(cpus), 64, s, n,
				    ns, "ns/roundtrip", ns / n);
		}
	}
	struct alignas(64) Line {
		std::atomic<u64> value[8]{};
	};
	static_assert(sizeof(Line) == 64);
	for (const auto &[name, cpus] : pairs(cores, cpu)) {
		for (bool padded : { false, true }) {
			for (int s = -1; s < samples; ++s) {
				std::array<Line, 2> lines;
				std::barrier gate(3);
				std::vector<std::thread> workers;
				const u64 increments = count(4000000);
				for (size_t t = 0; t < 2; ++t)
					workers.emplace_back([&, t] {
						pin(cpus[t]);
						auto &value =
							padded ?
								lines[t].value[0] :
								lines[0].value[t];
						gate.arrive_and_wait();
						gate.arrive_and_wait();
						for (u64 i = 0; i < increments;
						     ++i)
							value.fetch_add(
								1,
								std::memory_order_relaxed);
						gate.arrive_and_wait();
					});
				gate.arrive_and_wait();
				const auto start = Clock::now();
				gate.arrive_and_wait();
				gate.arrive_and_wait();
				const double ns = elapsed(start);
				for (auto &worker : workers)
					worker.join();
				const u64 a = lines[0].value[0],
					  b = padded ?
						      lines[1].value[0].load() :
						      lines[0].value[1].load();
				if (a != increments || b != increments)
					throw std::runtime_error(
						"atomic count mismatch");
				if (s >= 0)
					row("sharing",
					    name + (padded ? "_padded" :
							     "_packed"),
					    cpu_list(cpus), padded ? 128 : 64,
					    s, increments * 2, ns,
					    "ns/increment",
					    ns / (increments * 2));
			}
		}
	}
}

struct Pipe {
	int fd[2];
	Pipe()
	{
		if (pipe2(fd, O_CLOEXEC))
			fail("pipe2");
	}
	~Pipe()
	{
		close(fd[0]);
		close(fd[1]);
	}
};

void send_byte(int fd)
{
	ssize_t result;
	do {
		result = write(fd, "x", 1);
	} while (result < 0 && errno == EINTR);
	if (result != 1)
		fail("pipe write");
}

void recv_byte(int fd)
{
	char byte;
	ssize_t result;
	do {
		result = read(fd, &byte, 1);
	} while (result < 0 && errno == EINTR);
	if (result != 1)
		throw std::runtime_error("pipe read failed/EOF");
}

void context(const std::vector<Core> &cores, int cpu)
{
	auto configs = pairs(cores, cpu);
	configs.insert(configs.begin(), { "same_cpu", { cpu, cpu } });
	const u64 n = count(20000), warm = 1000;
	for (const auto &[name, cpus] : configs) {
		for (bool process : { false, true }) {
			for (int s = 0; s < samples; ++s) {
				Pipe request, reply;
				auto peer = [&] {
					pin(cpus[1]);
					for (u64 i = 0; i < n + warm; ++i) {
						recv_byte(request.fd[0]);
						send_byte(reply.fd[1]);
					}
				};
				std::thread worker;
				pid_t child = -1;
				if (process) {
					child = fork();
					if (child < 0)
						fail("fork");
					if (!child) {
						close(request.fd[1]);
						close(reply.fd[0]);
						peer();
						_exit(0);
					}
					close(request.fd[0]);
					request.fd[0] = -1;
					close(reply.fd[1]);
					reply.fd[1] = -1;
				} else
					worker = std::thread(peer);
				pin(cpus[0]);
				auto roundtrip = [&] {
					send_byte(request.fd[1]);
					recv_byte(reply.fd[0]);
				};
				for (u64 i = 0; i < warm; ++i)
					roundtrip();
				Usage before;
				const auto start = Clock::now();
				for (u64 i = 0; i < n; ++i)
					roundtrip();
				const double ns = elapsed(start);
				Usage after;
				if (process) {
					int status;
					if (waitpid(child, &status, 0) != child)
						fail("waitpid");
					if (!WIFEXITED(status) ||
					    WEXITSTATUS(status))
						throw std::runtime_error(
							"child failed");
				} else
					worker.join();
				const std::string label =
					name +
					(process ? "_process" : "_thread");
				row("context", label, cpu_list(cpus), 1, s, n,
				    ns, "ns/roundtrip", ns / n);
				row("context_switches", label, cpu_list(cpus),
				    1, s, n, ns, "initiator_nvcsw/roundtrip",
				    double(after.value.ru_nvcsw -
					   before.value.ru_nvcsw) /
					    n);
				row("context_switches", label, cpu_list(cpus),
				    1, s, n, ns, "initiator_nivcsw/roundtrip",
				    double(after.value.ru_nivcsw -
					   before.value.ru_nivcsw) /
					    n);
			}
		}
	}
}
} // namespace

int main(int argc, char **argv)
try {
	std::string group = "all";
	int cpu = -1;
	for (int i = 1; i < argc; ++i) {
		const std::string arg = argv[i];
		if (arg == "--quick")
			quick = true;
		else if (arg == "--help") {
			std::cout
				<< "microbench.out [--group all|compute|memory|syscall|scaling|coherence|context]"
				   " [--samples 1..30] [--cpu N] [--quick]\n"
				   "CSV on stdout; progress/PMU availability on stderr. Linux x86-64 only.\n";
			return 0;
		} else if (i + 1 < argc &&
			   (arg == "--group" || arg == "--samples" ||
			    arg == "--cpu")) {
			const std::string value = argv[++i];
			if (arg == "--group")
				group = value;
			else {
				size_t used = 0;
				const int number = std::stoi(value, &used);
				if (used != value.size() || number < 0)
					throw std::runtime_error(
						"invalid number: " + value);
				if (arg == "--samples")
					samples = number;
				else
					cpu = number;
			}
		} else
			throw std::runtime_error(
				"unknown or incomplete option: " + arg);
	}
	if (samples < 1 || samples > 30)
		throw std::runtime_error("samples must be 1..30");
	const std::vector<std::string> groups{ "compute",   "memory",
					       "syscall",   "scaling",
					       "coherence", "context" };
	if (group != "all" &&
	    std::find(groups.begin(), groups.end(), group) == groups.end())
		throw std::runtime_error("unknown group: " + group);
	const auto cores =
		topology(); // Discover before narrowing this thread's affinity.
	if (cpu == -1)
		cpu = cores.front().cpus[0];
	bool valid = false;
	for (const auto &core : cores)
		if (std::find(core.cpus.begin(), core.cpus.end(), cpu) !=
		    core.cpus.end())
			valid = true;
	if (!valid)
		throw std::runtime_error("CPU is outside initial affinity");
	pin(cpu);
	std::cout << std::fixed << std::setprecision(6);
	std::cout
		<< "group,case,cpus,working_set_bytes,sample,operations,elapsed_ns,unit,value\n";
	for (const auto &item : groups) {
		if (group != "all" && group != item)
			continue;
		pin(cpu);
		std::cerr << "running " << item << ", base CPU " << cpu
			  << ", samples " << samples << '\n';
		if (item == "compute")
			compute(cpu);
		else if (item == "memory")
			memory(cpu);
		else if (item == "syscall")
			syscalls(cpu);
		else if (item == "scaling")
			scaling(cores);
		else if (item == "coherence")
			coherence(cores, cpu);
		else if (item == "context")
			context(cores, cpu);
	}
	std::cerr << "checksum=" << sink << '\n';
	return 0;
} catch (const std::exception &error) {
	std::cerr << "error: " << error.what() << '\n';
	return 1;
}
