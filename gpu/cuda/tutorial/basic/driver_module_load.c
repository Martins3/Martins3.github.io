#include <cuda.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void check_driver(CUresult result, const char *what)
{
	if (result == CUDA_SUCCESS) {
		return;
	}

	const char *name = NULL;
	const char *message = NULL;
	cuGetErrorName(result, &name);
	cuGetErrorString(result, &message);
	fprintf(stderr, "[FAIL] %s: %s (%s)\n", what,
		name != NULL ? name : "UNKNOWN",
		message != NULL ? message : "unknown error");
	exit(1);
}

int main(void)
{
	enum { element_count = 1024, threads_per_block = 128 };
	const size_t bytes = sizeof(float) * element_count;
	const char *ptx_path = "./driver_module_kernel.ptx";
	float *host_a = NULL;
	float *host_b = NULL;
	float *host_c = NULL;
	CUdevice device;
	CUcontext context;
	CUmodule module;
	CUfunction function;
	CUdeviceptr dev_a;
	CUdeviceptr dev_b;
	CUdeviceptr dev_c;

	if (access("/dev/nvidiactl", F_OK) != 0) {
		fprintf(stderr, "[FAIL] /dev/nvidiactl is missing.\n");
		return 1;
	}

	host_a = (float *)malloc(bytes);
	host_b = (float *)malloc(bytes);
	host_c = (float *)malloc(bytes);
	if (host_a == NULL || host_b == NULL || host_c == NULL) {
		fprintf(stderr, "[FAIL] malloc\n");
		return 1;
	}

	for (int i = 0; i < element_count; ++i) {
		host_a[i] = (float)i * 1.5f;
		host_b[i] = (float)(element_count - i) * 0.25f;
		host_c[i] = -1.0f;
	}

	check_driver(cuInit(0), "cuInit");
	check_driver(cuDeviceGet(&device, 0), "cuDeviceGet");
	check_driver(cuCtxCreate(&context, nullptr, 0, device), "cuCtxCreate");
	check_driver(cuModuleLoad(&module, ptx_path), "cuModuleLoad");
	check_driver(cuModuleGetFunction(&function, module, "driver_vec_add"),
		     "cuModuleGetFunction");

	check_driver(cuMemAlloc(&dev_a, bytes), "cuMemAlloc(dev_a)");
	check_driver(cuMemAlloc(&dev_b, bytes), "cuMemAlloc(dev_b)");
	check_driver(cuMemAlloc(&dev_c, bytes), "cuMemAlloc(dev_c)");

	check_driver(cuMemcpyHtoD(dev_a, host_a, bytes), "cuMemcpyHtoD(dev_a)");
	check_driver(cuMemcpyHtoD(dev_b, host_b, bytes), "cuMemcpyHtoD(dev_b)");

	void *args[] = { &dev_a, &dev_b, &dev_c, (void *)&(int){ element_count } };
	const unsigned int blocks_per_grid =
		(element_count + threads_per_block - 1) / threads_per_block;

	check_driver(cuLaunchKernel(function, blocks_per_grid, 1, 1,
				    threads_per_block, 1, 1, 0, 0, args, NULL),
		     "cuLaunchKernel");
	check_driver(cuCtxSynchronize(), "cuCtxSynchronize");
	check_driver(cuMemcpyDtoH(host_c, dev_c, bytes), "cuMemcpyDtoH(dev_c)");

	for (int i = 0; i < element_count; ++i) {
		const float expected = host_a[i] + host_b[i];
		if (fabsf(host_c[i] - expected) > 1e-5f) {
			fprintf(stderr,
				"[FAIL] result mismatch at %d: got %.6f expected %.6f\n",
				i, host_c[i], expected);
			return 1;
		}
	}

	printf("[OK] Driver API module load + kernel launch passed\n");
	printf("  PTX module     : %s\n", ptx_path);
	printf("  kernel         : driver_vec_add\n");
	printf("  launch config  : grid=%u block=%d\n", blocks_per_grid,
	       threads_per_block);
	printf("  sample result  : c[0]=%.2f c[%d]=%.2f c[%d]=%.2f\n", host_c[0],
	       element_count / 2, host_c[element_count / 2], element_count - 1,
	       host_c[element_count - 1]);

	cuMemFree(dev_a);
	cuMemFree(dev_b);
	cuMemFree(dev_c);
	cuModuleUnload(module);
	cuCtxDestroy(context);
	free(host_a);
	free(host_b);
	free(host_c);
	return 0;
}
