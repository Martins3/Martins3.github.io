# libvirt 处理 cpuinfo
## 再次列举一次
- https://cpuid.apps.poly.nomial.co.uk/

## 只是查询一个 leaf
```txt
cpuid -l 1 -1
```
cpuid -f 可以读取文件，所以可以修改一个字符，然后 diff cpuid -f 的输出

## guest kernel
guest flags : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx pdpe1gb rdtscp lm constant_tsc arch_perfmon rep_good nopl xtopology cpuid tsc_known_freq pni pclmulqdq vmx ssse3 fma cx16 pdcm pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm 3dnowprefetch cpuid_fault invpcid_single ssbd ibrs ibpb stibp ibrs_enhanced tpr_shadow vnmi flexpriority ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid rdseed adx smap clflushopt clwb sha_ni xsaveopt xsavec xgetbv1 xsaves avx_vnni arat umip pku ospke waitpkg gfni vaes vpclmulqdq rdpid movdiri movdir64b fsrm md_clear serialize arch_capabilities

host flags : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc art arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc cpuid aperfmperf tsc_known_freq pni pclmulqdq dtes64 monitor ds_cpl vmx smx est tm2 ssse3 sdbg fma cx16 xtpr pdcm pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch cpuid_fault epb invpcid_single ssbd ibrs ibpb stibp ibrs_enhanced tpr_shadow vnmi flexpriority ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid rdseed adx smap clflushopt clwb intel_pt sha_ni xsaveopt xsavec xgetbv1 xsaves split_lock_detect avx_vnni dtherm ida arat pln pts hwp hwp_notify hwp_act_window hwp_epp hwp_pkg_req hfi umip pku ospke waitpkg gfni vaes vpclmulqdq tme rdpid movdiri movdir64b fsrm md_clear serialize pconfig arch_lbr ibt flush_l1d arch_capabilities


会消失的 flags :
dts acpi tm pbe art pebs bts nonstop_tsc aperfmperf dtes64 monitor ds_cpl smx est tm2 sdbg xtpr epb intel_pt split_lock_detect dtherm ida pln pts hwp hwp_notify hwp_act_window hwp_epp hwp_pkg_req hfi tme pconfig arch_lbr ibt flush_l1d

- [ ] 而且还可以增加 flags

arch/x86/kernel/cpu/common.c  中通过 cpuid 指令获取:
```c
void get_cpu_cap(struct cpuinfo_x86 *c)
```

- 在 kvm_set_cpu_caps 中重置一下 cpuid 的

当 host 相同的时候，guest 中的 feature 到底是怎么样子的?
- kvm_dev_ioctl_get_cpuid
  - get_cpuid_func
    - `__do_cpuid_func`
      - 部分 cpuid 会被 cpuid_entry_override 修正


其实还存在一些模拟属性

## 应该 `kvm_arch_dev_ioctl` 才是关键吧

## 如何方便的获取到 guest 中的 cpuid 呀

## cpuid_query_maxphyaddr : 原来 la57 和 cpu 具体的物理位宽两个不同的属性啊


## 从 nested = 1 热迁移到 nested = 0 的环境中，在虚拟机中使用 qemu , qemu 会直接 crash

这个 invalid instruction 的操作是谁注入的? 是首先退出到 kvm 中，然后注入，还是说，直接在物理机中处理了 ?

如果是经过 kvm 的话，不可能执行什么指令都来检查一次吧！

原来 handle_vmxon 是在 和 `vmx_hardware_enable->kvm_cpu_vmxon` 相对应。

应该是存储在:
```c
struct kvm_vcpu_arch::governed_features
```
并不是

## 方便的使用的这个项目
- https://github.com/nihui/ruapu

## 理解下 virsh 是如何协商到 cpu model 的

- virsh domcapabilities

```txt
<domainCapabilities>
  <path>/home/martins3/.nix-profile/bin/qemu-system-x86_64</path>
  <domain>kvm</domain>
  <machine>pc-i440fx-8.1</machine>
  <arch>x86_64</arch>
  <vcpu max='255'/>
  <iothreads supported='yes'/>
  <os supported='yes'>
    <enum name='firmware'/>
    <loader supported='yes'>
      <enum name='type'>
        <value>rom</value>
        <value>pflash</value>
      </enum>
      <enum name='readonly'>
        <value>yes</value>
        <value>no</value>
      </enum>
      <enum name='secure'>
        <value>no</value>
      </enum>
    </loader>
  </os>
  <cpu>
    <mode name='host-passthrough' supported='yes'>
      <enum name='hostPassthroughMigratable'>
        <value>on</value>
        <value>off</value>
      </enum>
    </mode>
    <mode name='maximum' supported='yes'>
      <enum name='maximumMigratable'>
        <value>on</value>
        <value>off</value>
      </enum>
    </mode>
    <mode name='host-model' supported='yes'>
      <model fallback='forbid'>Snowridge</model>
      <vendor>Intel</vendor>
      <maxphysaddr mode='passthrough' limit='46'/>
      <feature policy='require' name='ss'/>
      <feature policy='require' name='vmx'/>
      <feature policy='require' name='fma'/>
      <feature policy='require' name='avx'/>
      <feature policy='require' name='f16c'/>
      <feature policy='require' name='hypervisor'/>
      <feature policy='require' name='tsc_adjust'/>
      <feature policy='require' name='bmi1'/>
      <feature policy='require' name='avx2'/>
      <feature policy='require' name='bmi2'/>
      <feature policy='require' name='invpcid'/>
      <feature policy='require' name='adx'/>
      <feature policy='require' name='pku'/>
      <feature policy='require' name='waitpkg'/>
      <feature policy='require' name='vaes'/>
      <feature policy='require' name='vpclmulqdq'/>
      <feature policy='require' name='rdpid'/>
      <feature policy='require' name='fsrm'/>
      <feature policy='require' name='md-clear'/>
      <feature policy='require' name='serialize'/>
      <feature policy='require' name='stibp'/>
      <feature policy='require' name='flush-l1d'/>
      <feature policy='require' name='avx-vnni'/>
      <feature policy='require' name='fsrs'/>
      <feature policy='require' name='xsaves'/>
      <feature policy='require' name='abm'/>
      <feature policy='require' name='invtsc'/>
      <feature policy='require' name='ibpb'/>
      <feature policy='require' name='ibrs'/>
      <feature policy='require' name='amd-stibp'/>
      <feature policy='require' name='amd-ssbd'/>
      <feature policy='require' name='rdctl-no'/>
      <feature policy='require' name='ibrs-all'/>
      <feature policy='require' name='skip-l1dfl-vmentry'/>
      <feature policy='require' name='mds-no'/>
      <feature policy='require' name='pschange-mc-no'/>
      <feature policy='require' name='sbdr-ssdp-no'/>
      <feature policy='require' name='fbsdp-no'/>
      <feature policy='require' name='psdp-no'/>
      <feature policy='disable' name='mpx'/>
      <feature policy='disable' name='cldemote'/>
      <feature policy='disable' name='core-capability'/>
      <feature policy='disable' name='split-lock-detect'/>
    </mode>
    <mode name='custom' supported='yes'>
      <model usable='yes' vendor='unknown'>qemu64</model>
      <model usable='yes' vendor='unknown'>qemu32</model>
      <model usable='no' vendor='AMD'>phenom</model>
      <model usable='yes' vendor='unknown'>pentium3</model>
      <model usable='yes' vendor='unknown'>pentium2</model>
      <model usable='yes' vendor='unknown'>pentium</model>
      <model usable='yes' vendor='Intel'>n270</model>
      <model usable='yes' vendor='unknown'>kvm64</model>
      <model usable='yes' vendor='unknown'>kvm32</model>
      <model usable='yes' vendor='Intel'>coreduo</model>
      <model usable='yes' vendor='Intel'>core2duo</model>
      <model usable='no' vendor='AMD'>athlon</model>
      <model usable='yes' vendor='Intel'>Westmere-IBRS</model>
      <model usable='yes' vendor='Intel'>Westmere</model>
      <model usable='no' vendor='Intel'>Snowridge</model>
      <model usable='no' vendor='Intel'>Skylake-Server-noTSX-IBRS</model>
      <model usable='no' vendor='Intel'>Skylake-Server-IBRS</model>
      <model usable='no' vendor='Intel'>Skylake-Server</model>
      <model usable='no' vendor='Intel'>Skylake-Client-noTSX-IBRS</model>
      <model usable='no' vendor='Intel'>Skylake-Client-IBRS</model>
      <model usable='no' vendor='Intel'>Skylake-Client</model>
      <model usable='no' vendor='Intel'>SapphireRapids</model>
      <model usable='yes' vendor='Intel'>SandyBridge-IBRS</model>
      <model usable='yes' vendor='Intel'>SandyBridge</model>
      <model usable='yes' vendor='Intel'>Penryn</model>
      <model usable='no' vendor='AMD'>Opteron_G5</model>
      <model usable='no' vendor='AMD'>Opteron_G4</model>
      <model usable='no' vendor='AMD'>Opteron_G3</model>
      <model usable='yes' vendor='AMD'>Opteron_G2</model>
      <model usable='yes' vendor='AMD'>Opteron_G1</model>
      <model usable='yes' vendor='Intel'>Nehalem-IBRS</model>
      <model usable='yes' vendor='Intel'>Nehalem</model>
      <model usable='yes' vendor='Intel'>IvyBridge-IBRS</model>
      <model usable='yes' vendor='Intel'>IvyBridge</model>
      <model usable='no' vendor='Intel'>Icelake-Server-noTSX</model>
      <model usable='no' vendor='Intel'>Icelake-Server</model>
      <model usable='no' vendor='Intel'>Haswell-noTSX-IBRS</model>
      <model usable='no' vendor='Intel'>Haswell-noTSX</model>
      <model usable='no' vendor='Intel'>Haswell-IBRS</model>
      <model usable='no' vendor='Intel'>Haswell</model>
      <model usable='no' vendor='AMD'>EPYC-Rome</model>
      <model usable='no' vendor='AMD'>EPYC-Milan</model>
      <model usable='no' vendor='AMD'>EPYC-IBPB</model>
      <model usable='no' vendor='AMD'>EPYC-Genoa</model>
      <model usable='no' vendor='AMD'>EPYC</model>
      <model usable='no' vendor='Hygon'>Dhyana</model>
      <model usable='no' vendor='Intel'>Cooperlake</model>
      <model usable='yes' vendor='Intel'>Conroe</model>
      <model usable='no' vendor='Intel'>Cascadelake-Server-noTSX</model>
      <model usable='no' vendor='Intel'>Cascadelake-Server</model>
      <model usable='no' vendor='Intel'>Broadwell-noTSX-IBRS</model>
      <model usable='no' vendor='Intel'>Broadwell-noTSX</model>
      <model usable='no' vendor='Intel'>Broadwell-IBRS</model>
      <model usable='no' vendor='Intel'>Broadwell</model>
      <model usable='yes' vendor='unknown'>486</model>
    </mode>
  </cpu>
  <memoryBacking supported='yes'>
    <enum name='sourceType'>
      <value>file</value>
      <value>anonymous</value>
      <value>memfd</value>
    </enum>
  </memoryBacking>
  <devices>
    <disk supported='yes'>
      <enum name='diskDevice'>
        <value>disk</value>
        <value>cdrom</value>
        <value>floppy</value>
        <value>lun</value>
      </enum>
      <enum name='bus'>
        <value>ide</value>
        <value>fdc</value>
        <value>scsi</value>
        <value>virtio</value>
        <value>usb</value>
        <value>sata</value>
      </enum>
      <enum name='model'>
        <value>virtio</value>
        <value>virtio-transitional</value>
        <value>virtio-non-transitional</value>
      </enum>
    </disk>
    <graphics supported='yes'>
      <enum name='type'>
        <value>sdl</value>
        <value>vnc</value>
        <value>spice</value>
        <value>egl-headless</value>
        <value>dbus</value>
      </enum>
    </graphics>
    <video supported='yes'>
      <enum name='modelType'>
        <value>vga</value>
        <value>cirrus</value>
        <value>vmvga</value>
        <value>qxl</value>
        <value>virtio</value>
        <value>none</value>
        <value>bochs</value>
        <value>ramfb</value>
      </enum>
    </video>
    <hostdev supported='yes'>
      <enum name='mode'>
        <value>subsystem</value>
      </enum>
      <enum name='startupPolicy'>
        <value>default</value>
        <value>mandatory</value>
        <value>requisite</value>
        <value>optional</value>
      </enum>
      <enum name='subsysType'>
        <value>usb</value>
        <value>pci</value>
        <value>scsi</value>
      </enum>
      <enum name='capsType'/>
      <enum name='pciBackend'>
        <value>default</value>
        <value>vfio</value>
      </enum>
    </hostdev>
    <rng supported='yes'>
      <enum name='model'>
        <value>virtio</value>
        <value>virtio-transitional</value>
        <value>virtio-non-transitional</value>
      </enum>
      <enum name='backendModel'>
        <value>random</value>
        <value>egd</value>
        <value>builtin</value>
      </enum>
    </rng>
    <filesystem supported='yes'>
      <enum name='driverType'>
        <value>path</value>
        <value>handle</value>
        <value>virtiofs</value>
      </enum>
    </filesystem>
    <tpm supported='yes'>
      <enum name='model'>
        <value>tpm-tis</value>
        <value>tpm-crb</value>
      </enum>
      <enum name='backendModel'>
        <value>passthrough</value>
        <value>emulator</value>
        <value>external</value>
      </enum>
      <enum name='backendVersion'>
        <value>1.2</value>
        <value>2.0</value>
      </enum>
    </tpm>
    <redirdev supported='yes'>
      <enum name='bus'>
        <value>usb</value>
      </enum>
    </redirdev>
    <channel supported='yes'>
      <enum name='type'>
        <value>pty</value>
        <value>unix</value>
        <value>spicevmc</value>
      </enum>
    </channel>
    <crypto supported='yes'>
      <enum name='model'>
        <value>virtio</value>
      </enum>
      <enum name='type'>
        <value>qemu</value>
      </enum>
      <enum name='backendModel'>
        <value>builtin</value>
      </enum>
    </crypto>
  </devices>
  <features>
    <gic supported='no'/>
    <vmcoreinfo supported='yes'/>
    <genid supported='yes'/>
    <backingStoreInput supported='yes'/>
    <backup supported='yes'/>
    <async-teardown supported='yes'/>
    <sev supported='no'/>
    <sgx supported='no'/>
    <hyperv supported='yes'>
      <enum name='features'>
        <value>relaxed</value>
        <value>vapic</value>
        <value>spinlocks</value>
        <value>vpindex</value>
        <value>runtime</value>
        <value>synic</value>
        <value>stimer</value>
        <value>reset</value>
        <value>vendor_id</value>
        <value>frequencies</value>
        <value>reenlightenment</value>
        <value>tlbflush</value>
        <value>ipi</value>
        <value>evmcs</value>
        <value>avic</value>
      </enum>
    </hyperv>
  </features>
</domainCapabilities>
```

如果使用 sudo execsnoop 观察，可以看到:
```txt
/nix/store/0hz7gi3f5manp5fp4dpnisid9rzw5i50-qemu-8.1.5/bin/.qemu-system-x86_64-wrapped -S -no-user-config -nodefaults -nographic -machine none,accel=kvm:tcg -qmp unix:/home/martins3/.config/libvirt/qemu/lib/qmp-Z39ZN2/qmp.monitor,server=on,wait=off -pidfile /home/martins3/.config/libvirt/qemu/lib/qmp-Z39ZN2/qmp.pid -daemonize
```

也许还是调查下 virsh 的实现吧，一步步的走吧！
## 补充材料
https://github.com/qemu/qemu/blob/master/docs/system/arm/cpu-features.rst

## 随便看看的东西
https://news.ycombinator.com/item?id=33369901


## 整理一下这个东西
- virsh cpu-models x86_64
  - 应该展示的 qemu 支持的 x86_64 的所有的 model
- virsh domcapabilities
  - 最终调用为 qmp 的 query-cpu-definitions

- 本机不支持 rtm，如果强制使用 -cpu Skylake-Client-IBRS 启动，那么将会得到大量的警告

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
