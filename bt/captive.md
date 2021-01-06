## [captive](https://github.com/tspink/captive)
- main
  - KernelLoader::create_from_file
  - KVM::create_guest

platform :
1. symbench
2. user
3. virt

- [ ] I think code in arch are auto generated, but how are called from 

- [ ] captive only works on armv8, but no I don't know how to run it

binary translation 

## source code
- [ ] how engine loaded and executed ?

- Engine::load && Engine::install

`alloc_guest_memory(VM_PHYS_CODE_BASE, VM_CODE_SIZE, 0, (void *)0x670000000000);`

- [ ] how semihosting works ?


#### how unikernel works
/home/maritns3/core/captive-project/captive/src/hypervisor/kvm/cpu.cpp : here, we will start the unikernel, but how unikernel works ?

X86DAGEmitter::cmp_lt is called by arm64-jit-chunk-7.cpp, it's used for emit x86 code.

dbt : x86-emitter.c  x86-value.c and encoder.c

- [ ] VirtualRegisterAllocator and ReverseAllocator

- [ ] linux kernel setup : seems in kvm sregs setup.
  - [ ] what's kernel entry


after some setup, we came here :
/home/maritns3/core/captive-project/captive/arch/common/cpu-block-jit.cpp

## problem
- [ ] 100% CPU

- [ ] why so many code about devices ?

## only one thing is important now, the page walk

