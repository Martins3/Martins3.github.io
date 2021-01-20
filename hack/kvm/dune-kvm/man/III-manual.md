# MIPS® Architecture Reference Manual Vol. III: MIPS64® / microMIPS64™ Privileged Resource Architecture

#### 6.2.11 TLB Refill and XTLB Refill Exceptions

### 9.40 Exception Program Counter (CP0 Register 14, Select 0)
The Exception Program Counter (EPC) is a read/write register that contains the address at which processing resumes 
after an exception has been serviced. All bits of the EPC register are significant and must be writable.

Unless the EXL bit in the Status register is already a 1, the processor writes the EPC register when an exception

### 9.59 XContext Register (CP0 Register 20, Select 0)
However, it is unlikely to be useful to software in the TLB Refill Handler.
The `XContext` register duplicates some of the information provided in the `BadVAddr` register


