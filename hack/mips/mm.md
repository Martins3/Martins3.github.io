## tlbex
```
 History:        #0
 Commit:         380cd582c08831217ae693c86411902e6300ba6b
 Author:         Huacai Chen <chenhc@lemote.com>
 Committer:      Ralf Baechle <ralf@linux-mips.org>
 Author Date:    Thu 03 Mar 2016 09:45:12 AM CST
 Committer Date: Fri 13 May 2016 08:02:15 PM CST

 MIPS: Loongson-3: Fast TLB refill handler

 Loongson-3A R2 has pwbase/pwfield/pwsize/pwctl registers in CP0 (this
 is very similar to HTW) and lwdir/lwpte/lddir/ldpte instructions which
 can be used for fast TLB refill.
```

MIPS64-III-Priviledge :

- [ ] lddir : get pte specified by `PWCtl` `PWBase` `PWField`, `PWSize` and pgd pointer
  - [ ] PWBase : *virtual*  base directy address 
  - loongson manual says : load pte as PWField and PWsize indicates

## build_loongson3_tlb_refill_handler

