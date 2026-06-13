## event injection

- [ ] vcpu_vmx::idt_vectoring_info; 在普通模式下的
- [ ] `vmcs12->idt_vectoring_info_field` : 在 nested 模式下

- [ ] 在 vmcs 中间，存在 IDT-vectoring information
  - [ ] 文档:
    - 25.9.3 Information for VM Exits That Occur During Event Delivery
    - 27.5 EVENT INJECTION
    - 28.2.2 Information for VM Exits Due to Vectored Events
    - 28.2.4 Information for VM Exits During Event Delivery

中间指出，在处理一个 event delivery 的时候，
结果造成了另一个 vmexit, 那么这个 vmexit 出来的时候，
在 vmcs 中间的 idt_vectoring_info 就会存储下来当时正在 deliver 的 event，具体描述如下:

> - A VMExit can occur during event-delivery
> - Example: Write of exception frame to stack triggers `EPT_VIOLATION`
> - CPU saves the event which attempted to deliver in `vmcs->idt_vectoring_info`
> - On `guest->host`:
>   1. KVM checks if `vmcs->idt_vectoring_info valid`
>   2. If valid, queue injected event in `struct kvm_vcpu_arch` and set `KVM_REQ_EVENT`
> - `KVM_REQ_EVENT` will evaluate injected event on next entry to guest

如果错误注入的时候，发生了 vmexit ，那么重新注入。 具体工作在 `__vmx_complete_interrupts` 中进行

机制是 event deliver 的时候出现 vmexit，
到时候从 vmexit 的位置重新进入到 deliver 的 event 对应的 handler 不就完成了，
这个操作看来是重新进行一次 handler 执行

> What if VMExit occurs during event-delivery to L2?

- [ ] 需要重复注入才对吧

## 关键参考
- https://events19.linuxfoundation.org/wp-content/uploads/2017/12/Improving-KVM-x86-Nested-Virtualization-Liran-Alon-Oracle.pdf

- https://lore.kernel.org/kvm/20241015195227.GA18617@dev-dsk-iorlov-1b-d2eae488.eu-west-1.amazon.com/T/#m0e2931815a55992f1e3ffe3f1724f8948c7d3fdc

> Currently, the situation when guest accesses MMIO during event delivery
> is handled differently in VMX and SVM: on VMX KVM returns internal error
> with suberror = KVM_INTERNAL_ERROR_DELIVERY_EV, when SVM simply goes
> into infinite loop trying to deliver an event again and again.
>
> Such a situation could happen when the exception occurs with guest IDTR
> (or GDTR) descriptor base pointing to an MMIO address.

从这里的描述，更加处理中断的时候，遇到了错误

- https://bugzilla.kernel.org/show_bug.cgi?id=218267

## 其实也是符合预期的
时钟中断，触发 ept violation ，中断的处理过程，一些物理地址不存在，所以，

因为 external interrupt

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
