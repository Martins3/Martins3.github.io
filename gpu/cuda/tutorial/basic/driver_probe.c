/*
 *
 * 这个文件实际上已经把 Driver API 的最小闭环做出来了：
 *
 * - `dlopen(libcuda.so.1)`
 * - `dlsym(cuInit)`
 * - `cuInit(0)`
 * - `cuDeviceGetCount`
 * - `cuDeviceGet`
 * - `cuDeviceGetName`
 * - `cuDeviceGetAttribute`
 *
 * 它的工程价值很高，因为它清楚地说明：
 *
 * - 就算你不写 `__global__` kernel
 * - 只做 Driver API 初始化和设备枚举
 * - 也已经是在和 CUDA 的低层接口打交道
 *
 */
#include <cuda.h>

#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>

typedef CUresult (*cu_init_fn)(unsigned int);
typedef CUresult (*cu_device_get_count_fn)(int *);
typedef CUresult (*cu_device_get_fn)(CUdevice *, int);
typedef CUresult (*cu_device_get_name_fn)(char *, int, CUdevice);
typedef CUresult (*cu_device_get_attribute_fn)(int *, CUdevice_attribute, CUdevice);
typedef CUresult (*cu_get_error_name_fn)(CUresult, const char **);
typedef CUresult (*cu_get_error_string_fn)(CUresult, const char **);

static cu_init_fn cu_init_ptr;
static cu_device_get_count_fn cu_device_get_count_ptr;
static cu_device_get_fn cu_device_get_ptr;
static cu_device_get_name_fn cu_device_get_name_ptr;
static cu_device_get_attribute_fn cu_device_get_attribute_ptr;
static cu_get_error_name_fn cu_get_error_name_ptr;
static cu_get_error_string_fn cu_get_error_string_ptr;

static const char *result_name(CUresult result) {
    const char *name = NULL;
    if (cu_get_error_name_ptr == NULL) {
        return "UNKNOWN";
    }
    if (cu_get_error_name_ptr(result, &name) != CUDA_SUCCESS || name == NULL) {
        return "UNKNOWN";
    }
    return name;
}

static const char *result_string(CUresult result) {
    const char *message = NULL;
    if (cu_get_error_string_ptr == NULL) {
        return "unknown error";
    }
    if (cu_get_error_string_ptr(result, &message) != CUDA_SUCCESS || message == NULL) {
        return "unknown error";
    }
    return message;
}

int main(void) {
    void *libcuda = NULL;
    CUresult ret;
    const char *candidates[] = {
        "libcuda.so.1",
        "/usr/lib64/libcuda.so.1",
        "/lib64/libcuda.so.1",
    };
    size_t i;

    if (access("/dev/nvidiactl", F_OK) != 0) {
        fprintf(stderr, "[FAIL] /dev/nvidiactl is missing.\n");
        fprintf(stderr, "The CUDA userspace libraries are installed, but the device nodes are not available in this environment.\n");
        return 1;
    }

    for (i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
        libcuda = dlopen(candidates[i], RTLD_NOW);
        if (libcuda != NULL) {
            break;
        }
    }

    if (libcuda == NULL) {
        fprintf(stderr, "[FAIL] dlopen(libcuda.so.1): %s\n", dlerror());
        return 1;
    }

    cu_init_ptr = (cu_init_fn)dlsym(libcuda, "cuInit");
    cu_device_get_count_ptr = (cu_device_get_count_fn)dlsym(libcuda, "cuDeviceGetCount");
    cu_device_get_ptr = (cu_device_get_fn)dlsym(libcuda, "cuDeviceGet");
    cu_device_get_name_ptr = (cu_device_get_name_fn)dlsym(libcuda, "cuDeviceGetName");
    cu_device_get_attribute_ptr =
        (cu_device_get_attribute_fn)dlsym(libcuda, "cuDeviceGetAttribute");
    cu_get_error_name_ptr = (cu_get_error_name_fn)dlsym(libcuda, "cuGetErrorName");
    cu_get_error_string_ptr = (cu_get_error_string_fn)dlsym(libcuda, "cuGetErrorString");

    if (cu_init_ptr == NULL || cu_device_get_count_ptr == NULL || cu_device_get_ptr == NULL ||
        cu_device_get_name_ptr == NULL || cu_device_get_attribute_ptr == NULL) {
        fprintf(stderr, "[FAIL] Missing CUDA Driver API symbols.\n");
        dlclose(libcuda);
        return 1;
    }

    ret = cu_init_ptr(0);
    if (ret != CUDA_SUCCESS) {
        fprintf(stderr, "[FAIL] cuInit: %s (%s)\n", result_name(ret), result_string(ret));
        dlclose(libcuda);
        return 1;
    }

    int count = 0;
    ret = cu_device_get_count_ptr(&count);
    if (ret != CUDA_SUCCESS) {
        fprintf(stderr, "[FAIL] cuDeviceGetCount: %s (%s)\n", result_name(ret), result_string(ret));
        dlclose(libcuda);
        return 1;
    }

    printf("[OK] CUDA driver initialized, device count = %d\n", count);
    for (int i = 0; i < count; ++i) {
        CUdevice dev;
        char name[256] = {0};
        int major = 0;
        int minor = 0;

        ret = cu_device_get_ptr(&dev, i);
        if (ret != CUDA_SUCCESS) {
            fprintf(stderr, "[FAIL] cuDeviceGet(%d): %s (%s)\n", i, result_name(ret), result_string(ret));
            dlclose(libcuda);
            return 1;
        }

        cu_device_get_name_ptr(name, sizeof(name), dev);
        cu_device_get_attribute_ptr(&major, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, dev);
        cu_device_get_attribute_ptr(&minor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, dev);
        printf("  - device %d: %s, compute capability %d.%d\n", i, name, major, minor);
    }

    dlclose(libcuda);
    return 0;
}
