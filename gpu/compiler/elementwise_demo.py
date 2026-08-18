import tilelang
import tilelang.language as T
import time

@tilelang.jit(out_idx=[-1])
def elementwise_add(M, N, block_M, block_N, in_dtype, out_dtype, threads):
    @T.prim_func
    def elem_add(A: T.Tensor((M, N), in_dtype), B: T.Tensor((M, N), in_dtype), C: T.Tensor((M, N), out_dtype)):
        with T.Kernel(T.ceildiv(N, block_N), T.ceildiv(M, block_M), threads=threads) as (bx, by):
            A_shared = T.alloc_shared((block_M, block_N), in_dtype)
            B_shared = T.alloc_shared((block_M, block_N), in_dtype)
            C_local = T.alloc_fragment((block_M, block_N), out_dtype)
            C_shared = T.alloc_shared((block_M, block_N), out_dtype)

            T.copy(A[by * block_M, bx * block_N], A_shared)
            T.copy(B[by * block_M, bx * block_N], B_shared)
            for local_y, local_x in T.Parallel(block_M, block_N):
                C_local[local_y, local_x] = A_shared[local_y, local_x] + B_shared[local_y, local_x]
            T.copy(C_local, C_shared)
            T.copy(C_shared, C[by * block_M, bx * block_N])

    return elem_add

import torch
M, N = 1024, 1024
kernel = elementwise_add(M, N, block_M=32, block_N=32, threads=128, in_dtype=T.float32, out_dtype=T.float32)
print(kernel.get_kernel_source())
a = torch.randn(M, N, dtype=torch.float32, device="cuda")
b = torch.randn(M, N, dtype=torch.float32, device="cuda")
c = kernel(a, b)
ref_c = a + b
time.sleep(100);
torch.testing.assert_close(c, ref_c, rtol=1e-2, atol=1e-2)
print("[Demo 1/2] Elementwise Add: PASSED")
