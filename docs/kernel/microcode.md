- https://superuser.com/questions/1283788/what-exactly-is-microcode-and-how-does-it-differ-from-firmware
  - firmware 不一定是放到 ROM 中的，也可能直接在软件中
  - microcode 是 firmware 的一种
  - 修改 CPU 的指令映射
- https://winraid.level1techs.com/t/intel-amd-via-freescale-cpu-microcode-repositories-discussion/32301
  - 这里包含具体的资源

代码在这里
arch/x86/kernel/cpu/microcode/

https://superuser.com/questions/934752/do-arm-processors-like-cortex-a9-use-microcode


https://wiki.gentoo.org/wiki/AMD_microcode

### dracut 这个警告有意思

```txt
dracut --force --kver $(uname -r)
dracut: Disabling early microcode, because kernel does not support it. CONFIG_MICROCODE_[AMD|INTEL]!=y
```

## 看看
svm_check_emulate_instruction

```txt
	/*
	 * Detect and workaround Errata 1096 Fam_17h_00_0Fh.
	 *
	 * Errata:
	 * When CPU raises #NPF on guest data access and vCPU CR4.SMAP=1, it is
	 * possible that CPU microcode implementing DecodeAssist will fail to
	 * read guest memory at CS:RIP and vmcb.GuestIntrBytes will incorrectly
	 * be '0'.  This happens because microcode reads CS:RIP using a _data_
	 * loap uop with CPL=0 privileges.  If the load hits a SMAP #PF, ucode
	 * gives up and does not fill the instruction bytes buffer.
	 *
	 * As above, KVM reaches this point iff the VM is an SEV guest, the CPU
	 * supports DecodeAssist, a #NPF was raised, KVM's page fault handler
	 * triggered emulation (e.g. for MMIO), and the CPU returned 0 in the
	 * GuestIntrBytes field of the VMCB.
	 *
	 * This does _not_ mean that the erratum has been encountered, as the
	 * DecodeAssist will also fail if the load for CS:RIP hits a legitimate
	 * #PF, e.g. if the guest attempt to execute from emulated MMIO and
	 * encountered a reserved/not-present #PF.
	 *
	 * To hit the erratum, the following conditions must be true:
	 *    1. CR4.SMAP=1 (obviously).
	 *    2. CR4.SMEP=0 || CPL=3.  If SMEP=1 and CPL<3, the erratum cannot
	 *       have been hit as the guest would have encountered a SMEP
	 *       violation #PF, not a #NPF.
	 *    3. The #NPF is not due to a code fetch, in which case failure to
	 *       retrieve the instruction bytes is legitimate (see abvoe).
	 *
	 * In addition, don't apply the erratum workaround if the #NPF occurred
	 * while translating guest page tables (see below).
	 */
```

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
