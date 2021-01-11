## loongson manual
Root register update list: with virtualization support, vz add some function to CP0 register.

# virtualization manual
*4.4.3.2 Entry to Guest mode*
The recommended method of entering Guest mode is by executing an **ERET** instruction when Root.GuestCtl0GM=1,
Root.StatusEXL=1, Root.StatusERL=0 and Root.DebugDM=0


*4.4.3.3 Exit from Guest mode*

When an interrupt or exception is to be taken in root mode, the bits Root.StatusEXL or Root.StatusERL are set on entry,
before any machine state is saved. As a result, execution of the handler will take place in root mode, and root mode
exception context registers are used, including `Root.EPC`, `Root.Cause`, `Root.BadVAddr`, `Root.Context`, `Root.XContext`,
`Root.EntryHi`
- [ ] so we should preserve these root register before entering the host.

## 4.6 Coprocessor 0
Guest CP0 registers can be accessed from root mode by using the root-only MFGC0 and MTGC0 instructions. Doubleword access to guest CP0 registers is performed using the root-only DMFGC0 and DMTGC0 instructions.
Guest TLB contents can be accessed by using the root-only TLBGP, TLBGR, TLBGWI and TLBGWR instructions.
- [ ] 如果 guestid 存在，那么为什么 Guest TLB 有必要单独存在吗 ?

Root context software (hypervisor) is required to manage the initial state of writable Guest context registers. On
power-up, the initial state defaults to the hardware reset state as defined in the base architecture. On Guest context
save and restore, the hypervisor is required to preserve and re-initialize the Guest state. *For virtual boot of a Guest,
the hypervisor is required to initialize the Guest state equivalent to the hardware reset state.*
- [ ] 所以，这是 kvmtool 的工作 ?

- [ ] 4.7.3 Guest initiated Root TLB Exception

*4.7.1 Exceptions in Guest Mode*
The ‘onion model’ requires that every guest-mode operation be checked first against the guest CP0 context, and then
against the root CP0 context. Exceptions resulting from the guest CP0 context can be handled entirely within guest
mode without root-mode intervention. Exceptions resulting from the root-mode CP0 context (including GuestCtl0
permissions) require a root mode (hypervisor) handler.
