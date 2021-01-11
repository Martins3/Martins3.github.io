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

*4.7.1 Exceptions in Guest Mode*
The ‘onion model’ requires that every guest-mode operation be checked first against the guest CP0 context, and then
against the root CP0 context. Exceptions resulting from the guest CP0 context can be handled entirely within guest
mode without root-mode intervention. Exceptions resulting from the root-mode CP0 context (including GuestCtl0
permissions) require a root mode (hypervisor) handler.

