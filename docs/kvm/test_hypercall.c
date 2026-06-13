#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

// 测试在虚拟机中执行 hypercall

// KVM hypercall 号
#define KVM_HC_VAPIC_POLL_IRQ    0
#define KVM_HC_SEND_IPI          1

// x86 hypercall 指令
#ifdef __x86_64__
static inline long kvm_hypercall0(unsigned int nr)
{
    long ret;
    asm volatile(
        "vmcall"
        : "=a"(ret)
        : "a"(nr)
        : "memory"
    );
    return ret;
}

static inline long kvm_hypercall1(unsigned int nr, unsigned long a0)
{
    long ret;
    asm volatile(
        "vmcall"
        : "=a"(ret)
        : "a"(nr), "b"(a0)
        : "memory"
    );
    return ret;
}
#endif

int main() {
    printf("Testing hypercall in VM...\n");
    printf("Current PID: %d\n", getpid());
    
    // 检查是否在虚拟机中
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp) {
        char line[256];
        int is_vm = 0;
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "hypervisor")) {
                printf("Detected hypervisor flag in cpuinfo\n");
                is_vm = 1;
                break;
            }
        }
        fclose(fp);
        
        if (!is_vm) {
            printf("Warning: Not running in a VM, hypercall may fail\n");
        }
    }
    
#ifdef __x86_64__
    printf("Attempting hypercall 0...\n");
    
    // 尝试执行 hypercall
    // 注意：这可能触发 SIGILL 如果不在 VM 中
    long ret = kvm_hypercall0(KVM_HC_VAPIC_POLL_IRQ);
    
    printf("Hypercall returned: %ld\n", ret);
#else
    printf("Not x86_64 architecture, skipping hypercall test\n");
#endif
    
    return 0;
}
