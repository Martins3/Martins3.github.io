// GPU microbenchmarks. See README.md for measurement boundaries.
#pragma once

#include <cstdint>
#include <string>

#include <cuda_runtime.h>

using u64 = std::uint64_t;
using u32 = std::uint32_t;

struct Options {
	int device = 0;
	int samples = 5;
	bool quick = false;
	const char *group = nullptr; // nullptr = every group
};
extern Options opt;

[[noreturn]] void fail(const std::string &what);
[[noreturn]] void fail_cuda(cudaError_t e, const char *expr, const char *file,
			    int line);
inline void cuda_check(cudaError_t e, const char *expr, const char *file,
		       int line)
{
	if (e != cudaSuccess)
		fail_cuda(e, expr, file, line);
}
#define CUDA_CHECK(x) cuda_check((x), #x, __FILE__, __LINE__)

// Scale a full-length trip count down for --quick.
u64 count(u64 full);
// Host wall clock in nanoseconds, steady.
double cpu_ns();

// CSV: group,case,config,working_set_bytes,sample,operations,elapsed_ns,unit,value
// operations is always the denominator of value. timed_thr reports
// operations/elapsed_ns (throughput in Gflop/s, Gop/s, GB/s); timed_lat reports
// elapsed_ns/operations (latency in ns/update, ns/load, ns/transfer, ...).
// Both warm up once outside the timed region and emit one row per sample.
void row(const std::string &group, const std::string &name,
	 const std::string &config, u64 bytes, int sample, u64 operations,
	 double ns, const std::string &unit, double value);

// Event pair around a device region. stop_ns() synchronises on the end event.
struct GpuTimer {
	cudaEvent_t a{}, b{};
	GpuTimer();
	~GpuTimer();
	GpuTimer(const GpuTimer &) = delete;
	GpuTimer &operator=(const GpuTimer &) = delete;
	void start(cudaStream_t s = 0);
	double stop_ns(cudaStream_t s = 0);
};

template <class F>
void timed_thr(const char *group, const char *kase, const char *config, u64 ws,
	       u64 ops, const char *unit, F &&launch)
{
	for (int s = 0; s < opt.samples; s++) {
		launch();
		CUDA_CHECK(cudaDeviceSynchronize());
		GpuTimer t;
		t.start();
		launch();
		double ns = t.stop_ns();
		row(group, kase, config, ws, s, ops, ns, unit, ops / ns);
	}
}

template <class F>
void timed_lat(const char *group, const char *kase, const char *config, u64 ws,
	       u64 ops, const char *unit, F &&launch)
{
	for (int s = 0; s < opt.samples; s++) {
		launch();
		CUDA_CHECK(cudaDeviceSynchronize());
		GpuTimer t;
		t.start();
		launch();
		double ns = t.stop_ns();
		row(group, kase, config, ws, s, ops, ns, unit, ns / ops);
	}
}

// Stream a large buffer through every SM so later measurements see a cold L2.
void flush_l2(cudaStream_t s = 0);

// Hold a compute load on the device for `seconds` so the SM clocks leave the
// idle P-state. A consumer card boosts from idle over tens of milliseconds;
// without this the first throughput case of a group is a clock-ramp number.
void warm_clocks(double seconds);

// NVML sampling for A2 and for the PCIe link state. No-ops without NVML.
struct Telemetry {
	double sm_mhz = 0;
	double mem_mhz = 0;
	double power_w = 0;
	double temp_c = 0;
	int pcie_gen = 0;
	int pcie_width = 0;
};
bool telem_init();
bool telem_sample(Telemetry *out);
void telem_shutdown();

// Feasible interval for (steady_clock - %globaltimer), in nanoseconds.
struct ClockSync {
	double offset_ns = 0; // add to a GPU timestamp to obtain cpu_ns()
	double window_ns = 0; // width of the feasible interval; small means good
};
ClockSync calibrate_clocks();

void group_device();
void group_compute();
void group_memory();
void group_exec();
void group_launch();
void group_pcie();
