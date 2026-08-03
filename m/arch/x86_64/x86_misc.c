#include "internal.h"
#include <asm/nmi.h>
#include <asm/cpufeature.h>
#include <linux/mod_devicetable.h>
#include <asm/cpu_device_id.h>

// 1. 测试 boot_cpu_data 的访问
static int test_boot_cpu_data(void)
{
	// 输出 [92024.326865] [martins3:test_boot_cpu_data:6] 0xb7
	pr_info("x86_model 0x%x\n", boot_cpu_data.x86_model);

	// 参考 x86_match_cpu() ，分析其中的
	// struct cpuinfo_x86 *c = &boot_cpu_data;
	// cpu_has(c, X86_FEATURE_XSTORE);
	pr_info("cpu has feature : %d\n", boot_cpu_has(X86_FEATURE_XSTORE));

	struct x86_cpu_id via_rng_ids[] = {
		{ X86_VENDOR_CENTAUR, 6, X86_MODEL_ANY, X86_FEATURE_XSTORE }, {}
	};
	const struct x86_cpu_id *m = x86_match_cpu(via_rng_ids);
	pr_info("match result %px\n", m);
	return 0;
};

// 2. 测试 nmi 相关的东西，通过 qemu 注入 nmi
// [  113.900986] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.16.2-14-g1e1da7a96300-dirty-20250501_162647-nixos 04/01/2014
// [  113.900987] Call Trace:
// [  113.900989]  <NMI>
// [  113.900992]  dump_stack_lvl+0x53/0x70
// [  113.901002]  my_nmi_handler+0x1a/0x30 [martins3]
// [  113.901005]  nmi_handle+0x5e/0x150
// [  113.901010]  default_do_nmi+0x42/0x100
// [  113.901012]  exc_nmi+0xe0/0x110
// [  113.901013]  end_repeat_nmi+0xf/0x53
// [  113.901016] RIP: 0010:pv_native_safe_halt+0xf/0x20
// [  113.901018] Code: 62 4d 00 c3 cc cc cc cc 0f 1f 00 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 f3 0f 1e fa eb 07 0f 00 2d 25 1c 17 00 fb f4 <c3> cc cc cc cc 66 2e 0f 1f 84 00 00 00 00 00 66 90 90 90 90 90 90
// [  113.901019] RSP: 0018:ffffffff82403e88 EFLAGS: 00000286
// [  113.901020] RAX: ffff888237c00000 RBX: ffffffff8240c900 RCX: 0000000000000001
// [  113.901021] RDX: 0000000000000000 RSI: ffffffff81f5930b RDI: 0000000000037e34
// [  113.901021] RBP: 0000000000000000 R08: 0000000000037e34 R09: 0000000000000001
// [  113.901021] R10: 0000001a8f0acf40 R11: 0000000000000000 R12: 0000000000000000
// [  113.901022] R13: 0000000000000000 R14: ffffffff8240c048 R15: 0000000000014770
// [  113.901023]  ? pv_native_safe_halt+0xf/0x20
// [  113.901025]  ? pv_native_safe_halt+0xf/0x20
// [  113.901026]  </NMI>
// [  113.901026]  <TASK>
// [  113.901026]  default_idle+0x13/0x20
// [  113.901028]  default_idle_call+0x30/0xf0
// [  113.901030]  do_idle+0x1b5/0x200
// [  113.901033]  cpu_startup_entry+0x29/0x30
// [  113.901034]  rest_init+0xcc/0xd0
// [  113.901035]  start_kernel+0x4ef/0x7a0
// [  113.901039]  x86_64_start_reservations+0x18/0x30
// [  113.901041]  x86_64_start_kernel+0xc5/0xd0
// [  113.901042]  common_startup_64+0x13e/0x148
// [  113.901045]  </TASK>
//
// TODO init_hw_perf_events dump 一下内容吧，所以这里有一个问题，
// 似乎可以控制 nmi 的主动触发?
static int my_nmi_handler(unsigned int val, struct pt_regs *regs)

{
	pr_emerg("Custom NMI handler triggered!\n");
	dump_stack();
	return NMI_HANDLED;
}

int test_x86_misc_init(void)
{
	register_nmi_handler(NMI_LOCAL, my_nmi_handler, 0, "my nmi");
	return 0;
}

int test_x86_misc_exit(void)
{
	unregister_nmi_handler(NMI_LOCAL, "my nmi");
	return 0;
}

int test_x86_misc(long action)
{
	switch (action) {
	case 0:
		test_boot_cpu_data();
		break;
	default:
		break;
	}
	return 0;
}
