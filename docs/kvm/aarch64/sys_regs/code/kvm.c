#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/kvm.h>
#include <asm/kvm.h>

#ifndef KVM_ARM_PREFERRED_TARGET
#define KVM_ARM_PREFERRED_TARGET _IOR(KVMIO, 0xaf, struct kvm_vcpu_init)
#endif

#ifndef KVM_ARM_VCPU_INIT
#define KVM_ARM_VCPU_INIT _IOW(KVMIO, 0xae, struct kvm_vcpu_init)
#endif

#ifndef KVM_CAP_ARM_PMU_V3
#define KVM_CAP_ARM_PMU_V3 168
#endif

#ifndef KVM_ARM_VCPU_PMU_V3
#define KVM_ARM_VCPU_PMU_V3 3
#endif

/* AArch64 ID register encodings for KVM_SET/KVM_GET_ONE_REG.
 * 0x60... prefix = KVM_REG_ARM64 | KVM_REG_SIZE_U64.
 */
/* Debug Feature Register 0 */
#define REG_ID_AA64DFR0_EL1 0x603000000013c028ULL
/* Memory Model Feature Register 1 */
#define REG_ID_AA64MMFR1_EL1 0x603000000013c039ULL
/* Instruction Set Attribute Register 2 */
#define REG_ID_AA64ISAR2_EL1 0x603000000013c032ULL

/* PMCR_EL0: op0=3, op1=3, crn=9, crm=12, op2=0 */
#define REG_PMCR_EL0 (KVM_REG_ARM64 | KVM_REG_SIZE_U64 | \
                      KVM_REG_ARM64_SYSREG | \
                      (3UL << KVM_REG_ARM64_SYSREG_OP0_SHIFT) | \
                      (3UL << KVM_REG_ARM64_SYSREG_OP1_SHIFT) | \
                      (9UL << KVM_REG_ARM64_SYSREG_CRN_SHIFT) | \
                      (12UL << KVM_REG_ARM64_SYSREG_CRM_SHIFT) | \
                      (0UL << KVM_REG_ARM64_SYSREG_OP2_SHIFT))

/* Read a 64-bit VCPU register. Returns 0 on success, -1 on error. */
static int get_one_reg(int vcpu, uint64_t id, uint64_t *val)
{
	struct kvm_one_reg reg = {
		.id = id,
		.addr = (uint64_t)val,
	};

	if (ioctl(vcpu, KVM_GET_ONE_REG, &reg) < 0) {
		perror("KVM_GET_ONE_REG");
		return -1;
	}
	return 0;
}

static void print_id_aa64dfr0(uint64_t val)
{
	printf("ID_AA64DFR0_EL1 = 0x%016llx\n", (unsigned long long)val);
	printf("  PMSVer   = 0x%llx\n",
	       (unsigned long long)((val >> 32) & 0xf));
	printf("  PMUVer   = 0x%llx\n", (unsigned long long)((val >> 8) & 0xf));
	printf("  DebugVer = 0x%llx\n", (unsigned long long)(val & 0xf));
}

static void print_id_aa64mmfr1(uint64_t val)
{
	printf("ID_AA64MMFR1_EL1 = 0x%016llx\n", (unsigned long long)val);
	printf("  XNX   = 0x%llx\n", (unsigned long long)((val >> 28) & 0xf));
	printf("  LO    = 0x%llx\n", (unsigned long long)((val >> 16) & 0xf));
	printf("  ECBHB = 0x%llx\n", (unsigned long long)((val >> 60) & 0xf));
}

static int create_vm_vcpu(int *out_kvm, int *out_vm, int *out_vcpu)
{
	int kvm = -1, vm = -1, vcpu = -1;
	struct kvm_vcpu_init init;
	int pmu_cap;

	kvm = open("/dev/kvm", O_RDWR | O_CLOEXEC);
	if (kvm < 0) {
		perror("open /dev/kvm");
		goto err;
	}

	pmu_cap = ioctl(kvm, KVM_CHECK_EXTENSION, KVM_CAP_ARM_PMU_V3);
	printf("KVM_CAP_ARM_PMU_V3 available: %d\n", pmu_cap);

	vm = ioctl(kvm, KVM_CREATE_VM, 0);
	if (vm < 0) {
		perror("KVM_CREATE_VM");
		goto err;
	}

	memset(&init, 0, sizeof(init));
	if (ioctl(vm, KVM_ARM_PREFERRED_TARGET, &init) < 0) {
		perror("KVM_ARM_PREFERRED_TARGET");
		goto err;
	}

	if (pmu_cap > 0) {
		init.features[0] |= (1U << KVM_ARM_VCPU_PMU_V3);
		printf("requesting KVM_ARM_VCPU_PMU_V3\n");
	}

	vcpu = ioctl(vm, KVM_CREATE_VCPU, 0);
	if (vcpu < 0) {
		perror("KVM_CREATE_VCPU");
		goto err;
	}

	if (ioctl(vcpu, KVM_ARM_VCPU_INIT, &init) < 0) {
		fprintf(stderr, "KVM_ARM_VCPU_INIT with PMU_V3 failed: %s\n", strerror(errno));
		if (pmu_cap > 0) {
			init.features[0] &= ~(1U << KVM_ARM_VCPU_PMU_V3);
			if (ioctl(vcpu, KVM_ARM_VCPU_INIT, &init) < 0) {
				fprintf(stderr, "KVM_ARM_VCPU_INIT without PMU_V3 also failed: %s\n", strerror(errno));
				goto err;
			}
			printf("VCPU init succeeded WITHOUT PMU_V3\n");
		} else {
			goto err;
		}
	} else {
		if (pmu_cap > 0)
			printf("VCPU init succeeded WITH KVM_ARM_VCPU_PMU_V3\n");
	}

	*out_kvm = kvm;
	*out_vm = vm;
	*out_vcpu = vcpu;
	return 0;

err:
	if (vcpu >= 0)
		close(vcpu);
	if (vm >= 0)
		close(vm);
	if (kvm >= 0)
		close(kvm);
	return -1;
}

int main(void)
{
	int kvm, vm, vcpu;
	uint64_t val;

	if (create_vm_vcpu(&kvm, &vm, &vcpu) < 0)
		return 1;

	if (get_one_reg(vcpu, REG_ID_AA64DFR0_EL1, &val) < 0)
		return 1;
	print_id_aa64dfr0(val);

	if (get_one_reg(vcpu, REG_ID_AA64MMFR1_EL1, &val) < 0)
		return 1;
	print_id_aa64mmfr1(val);

	if (get_one_reg(vcpu, REG_ID_AA64ISAR2_EL1, &val) < 0)
		return 1;
	return 0;
}
