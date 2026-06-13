# kvm 如何支持 smm

## 分析 qemu 一共会调用多少次 KVM_SET_USER_MEMORY_REGION

- [ ] smm 打开之后，会增加一些调用
- [ ] 为什么对于一个位置会反复的调用

smm off 的时候:
```txt
kvm_set_user_memory Slot#0 flags=0x0 gpa=0x0 size=0xc0000000 ua=0x7f3b17a00000 ret=0
kvm_set_user_memory Slot#1 flags=0x0 gpa=0x100000000 size=0x940000000 ua=0x7f3bd7a00000 ret=0
kvm_set_user_memory Slot#2 flags=0x2 gpa=0xfffc0000 size=0x40000 ua=0x7f3b14600000 ret=0
kvm_set_user_memory Slot#0 flags=0x0 gpa=0x0 size=0x0 ua=0x7f3b17a00000 ret=0
kvm_set_user_memory Slot#0 flags=0x0 gpa=0x0 size=0xc0000 ua=0x7f3b17a00000 ret=0
kvm_set_user_memory Slot#3 flags=0x2 gpa=0xc0000 size=0x20000 ua=0x7f3b14400000 ret=0
kvm_set_user_memory Slot#4 flags=0x2 gpa=0xe0000 size=0x20000 ua=0x7f3b14620000 ret=0
kvm_set_user_memory Slot#5 flags=0x0 gpa=0x100000 size=0xbff00000 ua=0x7f3b17b00000 ret=0
kvm_set_user_memory Slot#0 flags=0x0 gpa=0x0 size=0x0 ua=0x7f3b17a00000 ret=0
kvm_set_user_memory Slot#0 flags=0x0 gpa=0x0 size=0xa0000 ua=0x7f3b17a00000 ret=0
kvm_set_user_memory Slot#3 flags=0x0 gpa=0xc0000 size=0x0 ua=0x7f3b14400000 ret=0
kvm_set_user_memory Slot#4 flags=0x0 gpa=0xe0000 size=0x0 ua=0x7f3b14620000 ret=0
kvm_set_user_memory Slot#5 flags=0x0 gpa=0x100000 size=0x0 ua=0x7f3b17b00000 ret=0
kvm_set_user_memory Slot#3 flags=0x0 gpa=0xc0000 size=0x10000 ua=0x7f3b17ac0000 ret=0
kvm_set_user_memory Slot#4 flags=0x2 gpa=0xd0000 size=0x10000 ua=0x7f3b14410000 ret=0
kvm_set_user_memory Slot#5 flags=0x2 gpa=0xe0000 size=0x10000 ua=0x7f3b14620000 ret=0
kvm_set_user_memory Slot#6 flags=0x0 gpa=0xf0000 size=0xbff10000 ua=0x7f3b17af0000 ret=0
kvm_set_user_memory Slot#3 flags=0x0 gpa=0xc0000 size=0x0 ua=0x7f3b17ac0000 ret=0
kvm_set_user_memory Slot#4 flags=0x0 gpa=0xd0000 size=0x0 ua=0x7f3b14410000 ret=0
kvm_set_user_memory Slot#5 flags=0x0 gpa=0xe0000 size=0x0 ua=0x7f3b14620000 ret=0
kvm_set_user_memory Slot#6 flags=0x0 gpa=0xf0000 size=0x0 ua=0x7f3b17af0000 ret=0
kvm_set_user_memory Slot#3 flags=0x0 gpa=0xc0000 size=0xbff40000 ua=0x7f3b17ac0000 ret=0
kvm_set_user_memory Slot#4 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7f3a81600000 ret=0
kvm_set_user_memory Slot#5 flags=0x2 gpa=0xfea40000 size=0x10000 ua=0x7f3b14200000 ret=0
kvm_set_user_memory Slot#5 flags=0x0 gpa=0xfea40000 size=0x0 ua=0x7f3b14200000 ret=0
kvm_set_user_memory Slot#5 flags=0x2 gpa=0xfea00000 size=0x40000 ua=0x7f3ab8600000 ret=0
kvm_set_user_memory Slot#5 flags=0x0 gpa=0xfea00000 size=0x0 ua=0x7f3ab8600000 ret=0
kvm_set_user_memory Slot#3 flags=0x0 gpa=0xc0000 size=0x0 ua=0x7f3b17ac0000 ret=0
kvm_set_user_memory Slot#3 flags=0x0 gpa=0xc0000 size=0xb000 ua=0x7f3b17ac0000 ret=0
kvm_set_user_memory Slot#5 flags=0x0 gpa=0xcb000 size=0x3000 ua=0x7f3b17acb000 ret=0
kvm_set_user_memory Slot#6 flags=0x0 gpa=0xce000 size=0x2000 ua=0x7f3b17ace000 ret=0
kvm_set_user_memory Slot#7 flags=0x0 gpa=0xd0000 size=0x20000 ua=0x7f3b17ad0000 ret=0
kvm_set_user_memory Slot#8 flags=0x0 gpa=0xf0000 size=0x10000 ua=0x7f3b17af0000 ret=0
kvm_set_user_memory Slot#9 flags=0x0 gpa=0x100000 size=0xbff00000 ua=0x7f3b17b00000 ret=0
kvm_set_user_memory Slot#6 flags=0x0 gpa=0xce000 size=0x0 ua=0x7f3b17ace000 ret=0
kvm_set_user_memory Slot#7 flags=0x0 gpa=0xd0000 size=0x0 ua=0x7f3b17ad0000 ret=0
kvm_set_user_memory Slot#6 flags=0x0 gpa=0xce000 size=0x1a000 ua=0x7f3b17ace000 ret=0
kvm_set_user_memory Slot#7 flags=0x0 gpa=0xe8000 size=0x8000 ua=0x7f3b17ae8000 ret=0
kvm_set_user_memory Slot#4 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7f3a81600000 ret=0
kvm_set_user_memory Slot#4 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7f3a81600000 ret=0
kvm_set_user_memory Slot#4 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7f3a81600000 ret=0
kvm_set_user_memory Slot#4 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7f3a81600000 ret=0
kvm_set_user_memory Slot#4 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7f3a81600000 ret=0
kvm_set_user_memory Slot#4 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7f3a81600000 ret=0
kvm_set_user_memory Slot#4 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7f3a81600000 ret=0
kvm_set_user_memory Slot#4 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7f3a81600000 ret=0
kvm_set_user_memory Slot#4 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7f3a81600000 ret=0
kvm_set_user_memory Slot#4 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7f3a81600000 ret=0
kvm_set_user_memory Slot#4 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7f3a81600000 ret=0
kvm_set_user_memory Slot#4 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7f3a81600000 ret=0
kvm_set_user_memory Slot#4 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7f3a81600000 ret=0
kvm_set_user_memory Slot#4 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7f3a81600000 ret=0
```

```txt
kvm_set_user_memory Slot#0 flags=0x0 gpa=0x0 size=0xc0000000 ua=0x7fb2a3e00000 ret=0
kvm_set_user_memory Slot#1 flags=0x0 gpa=0x100000000 size=0x940000000 ua=0x7fb363e00000 ret=0
kvm_set_user_memory Slot#2 flags=0x2 gpa=0xfffc0000 size=0x40000 ua=0x7fb2a0400000 ret=0
kvm_set_user_memory Slot#0 flags=0x0 gpa=0x0 size=0x0 ua=0x7fb2a3e00000 ret=0
kvm_set_user_memory Slot#0 flags=0x0 gpa=0x0 size=0xc0000 ua=0x7fb2a3e00000 ret=0
kvm_set_user_memory Slot#3 flags=0x2 gpa=0xc0000 size=0x20000 ua=0x7fb2a0200000 ret=0
kvm_set_user_memory Slot#4 flags=0x2 gpa=0xe0000 size=0x20000 ua=0x7fb2a0420000 ret=0
kvm_set_user_memory Slot#5 flags=0x0 gpa=0x100000 size=0xbff00000 ua=0x7fb2a3f00000 ret=0
kvm_set_user_memory Slot#0 flags=0x0 gpa=0x0 size=0x0 ua=0x7fb2a3e00000 ret=0
kvm_set_user_memory Slot#0 flags=0x0 gpa=0x0 size=0xa0000 ua=0x7fb2a3e00000 ret=0
kvm_set_user_memory Slot#65536 flags=0x0 gpa=0x0 size=0xc0000 ua=0x7fb2a3e00000 ret=0
kvm_set_user_memory Slot#65537 flags=0x2 gpa=0xc0000 size=0x20000 ua=0x7fb2a0200000 ret=0
kvm_set_user_memory Slot#65538 flags=0x2 gpa=0xe0000 size=0x20000 ua=0x7fb2a0420000 ret=0
kvm_set_user_memory Slot#65539 flags=0x0 gpa=0x100000 size=0xbff00000 ua=0x7fb2a3f00000 ret=0
kvm_set_user_memory Slot#65540 flags=0x2 gpa=0xfffc0000 size=0x40000 ua=0x7fb2a0400000 ret=0
kvm_set_user_memory Slot#65541 flags=0x0 gpa=0x100000000 size=0x940000000 ua=0x7fb363e00000 ret=0
kvm_set_user_memory Slot#3 flags=0x0 gpa=0xc0000 size=0x0 ua=0x7fb2a0200000 ret=0
kvm_set_user_memory Slot#4 flags=0x0 gpa=0xe0000 size=0x0 ua=0x7fb2a0420000 ret=0
kvm_set_user_memory Slot#5 flags=0x0 gpa=0x100000 size=0x0 ua=0x7fb2a3f00000 ret=0
kvm_set_user_memory Slot#3 flags=0x0 gpa=0xc0000 size=0x10000 ua=0x7fb2a3ec0000 ret=0
kvm_set_user_memory Slot#4 flags=0x2 gpa=0xd0000 size=0x10000 ua=0x7fb2a0210000 ret=0
kvm_set_user_memory Slot#5 flags=0x2 gpa=0xe0000 size=0x10000 ua=0x7fb2a0420000 ret=0
kvm_set_user_memory Slot#6 flags=0x0 gpa=0xf0000 size=0xbff10000 ua=0x7fb2a3ef0000 ret=0
kvm_set_user_memory Slot#65536 flags=0x0 gpa=0x0 size=0x0 ua=0x7fb2a3e00000 ret=0
kvm_set_user_memory Slot#65537 flags=0x0 gpa=0xc0000 size=0x0 ua=0x7fb2a0200000 ret=0
kvm_set_user_memory Slot#65538 flags=0x0 gpa=0xe0000 size=0x0 ua=0x7fb2a0420000 ret=0
kvm_set_user_memory Slot#65539 flags=0x0 gpa=0x100000 size=0x0 ua=0x7fb2a3f00000 ret=0
kvm_set_user_memory Slot#65536 flags=0x0 gpa=0x0 size=0xa0000 ua=0x7fb2a3e00000 ret=0
kvm_set_user_memory Slot#65537 flags=0x0 gpa=0xc0000 size=0x10000 ua=0x7fb2a3ec0000 ret=0
kvm_set_user_memory Slot#65538 flags=0x2 gpa=0xd0000 size=0x10000 ua=0x7fb2a0210000 ret=0
kvm_set_user_memory Slot#65539 flags=0x2 gpa=0xe0000 size=0x10000 ua=0x7fb2a0420000 ret=0
kvm_set_user_memory Slot#65542 flags=0x0 gpa=0xf0000 size=0xbff10000 ua=0x7fb2a3ef0000 ret=0
kvm_set_user_memory Slot#3 flags=0x0 gpa=0xc0000 size=0x0 ua=0x7fb2a3ec0000 ret=0
kvm_set_user_memory Slot#4 flags=0x0 gpa=0xd0000 size=0x0 ua=0x7fb2a0210000 ret=0
kvm_set_user_memory Slot#5 flags=0x0 gpa=0xe0000 size=0x0 ua=0x7fb2a0420000 ret=0
kvm_set_user_memory Slot#6 flags=0x0 gpa=0xf0000 size=0x0 ua=0x7fb2a3ef0000 ret=0
kvm_set_user_memory Slot#3 flags=0x0 gpa=0xc0000 size=0xbff40000 ua=0x7fb2a3ec0000 ret=0
kvm_set_user_memory Slot#65537 flags=0x0 gpa=0xc0000 size=0x0 ua=0x7fb2a3ec0000 ret=0
kvm_set_user_memory Slot#65538 flags=0x0 gpa=0xd0000 size=0x0 ua=0x7fb2a0210000 ret=0
kvm_set_user_memory Slot#65539 flags=0x0 gpa=0xe0000 size=0x0 ua=0x7fb2a0420000 ret=0
kvm_set_user_memory Slot#65542 flags=0x0 gpa=0xf0000 size=0x0 ua=0x7fb2a3ef0000 ret=0
kvm_set_user_memory Slot#65537 flags=0x0 gpa=0xc0000 size=0xbff40000 ua=0x7fb2a3ec0000 ret=0
kvm_set_user_memory Slot#4 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#65538 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#0 flags=0x0 gpa=0x0 size=0x0 ua=0x7fb2a3e00000 ret=0
kvm_set_user_memory Slot#3 flags=0x0 gpa=0xc0000 size=0x0 ua=0x7fb2a3ec0000 ret=0
kvm_set_user_memory Slot#0 flags=0x0 gpa=0x0 size=0xc0000000 ua=0x7fb2a3e00000 ret=0
kvm_set_user_memory Slot#65536 flags=0x0 gpa=0x0 size=0x0 ua=0x7fb2a3e00000 ret=0
kvm_set_user_memory Slot#65537 flags=0x0 gpa=0xc0000 size=0x0 ua=0x7fb2a3ec0000 ret=0
kvm_set_user_memory Slot#65536 flags=0x0 gpa=0x0 size=0xc0000000 ua=0x7fb2a3e00000 ret=0
kvm_set_user_memory Slot#0 flags=0x0 gpa=0x0 size=0x0 ua=0x7fb2a3e00000 ret=0
kvm_set_user_memory Slot#0 flags=0x0 gpa=0x0 size=0xa0000 ua=0x7fb2a3e00000 ret=0
kvm_set_user_memory Slot#3 flags=0x0 gpa=0xc0000 size=0xbff40000 ua=0x7fb2a3ec0000 ret=0
kvm_set_user_memory Slot#5 flags=0x2 gpa=0xfea40000 size=0x10000 ua=0x7fb280600000 ret=0
kvm_set_user_memory Slot#65537 flags=0x2 gpa=0xfea40000 size=0x10000 ua=0x7fb280600000 ret=0
kvm_set_user_memory Slot#5 flags=0x0 gpa=0xfea40000 size=0x0 ua=0x7fb280600000 ret=0
kvm_set_user_memory Slot#65537 flags=0x0 gpa=0xfea40000 size=0x0 ua=0x7fb280600000 ret=0
kvm_set_user_memory Slot#5 flags=0x2 gpa=0xfea00000 size=0x40000 ua=0x7fb240600000 ret=0
kvm_set_user_memory Slot#65537 flags=0x2 gpa=0xfea00000 size=0x40000 ua=0x7fb240600000 ret=0
kvm_set_user_memory Slot#5 flags=0x0 gpa=0xfea00000 size=0x0 ua=0x7fb240600000 ret=0
kvm_set_user_memory Slot#65537 flags=0x0 gpa=0xfea00000 size=0x0 ua=0x7fb240600000 ret=0
kvm_set_user_memory Slot#3 flags=0x0 gpa=0xc0000 size=0x0 ua=0x7fb2a3ec0000 ret=0
kvm_set_user_memory Slot#3 flags=0x0 gpa=0xc0000 size=0xb000 ua=0x7fb2a3ec0000 ret=0
kvm_set_user_memory Slot#5 flags=0x0 gpa=0xcb000 size=0x3000 ua=0x7fb2a3ecb000 ret=0
kvm_set_user_memory Slot#6 flags=0x0 gpa=0xce000 size=0x2000 ua=0x7fb2a3ece000 ret=0
kvm_set_user_memory Slot#7 flags=0x0 gpa=0xd0000 size=0x20000 ua=0x7fb2a3ed0000 ret=0
kvm_set_user_memory Slot#8 flags=0x0 gpa=0xf0000 size=0x10000 ua=0x7fb2a3ef0000 ret=0
kvm_set_user_memory Slot#9 flags=0x0 gpa=0x100000 size=0xbff00000 ua=0x7fb2a3f00000 ret=0
kvm_set_user_memory Slot#65536 flags=0x0 gpa=0x0 size=0x0 ua=0x7fb2a3e00000 ret=0
kvm_set_user_memory Slot#65536 flags=0x0 gpa=0x0 size=0xc0000 ua=0x7fb2a3e00000 ret=0
kvm_set_user_memory Slot#65537 flags=0x0 gpa=0xc0000 size=0xb000 ua=0x7fb2a3ec0000 ret=0
kvm_set_user_memory Slot#65539 flags=0x0 gpa=0xcb000 size=0x3000 ua=0x7fb2a3ecb000 ret=0
kvm_set_user_memory Slot#65542 flags=0x0 gpa=0xce000 size=0x2000 ua=0x7fb2a3ece000 ret=0
kvm_set_user_memory Slot#65543 flags=0x0 gpa=0xd0000 size=0x20000 ua=0x7fb2a3ed0000 ret=0
kvm_set_user_memory Slot#65544 flags=0x0 gpa=0xf0000 size=0x10000 ua=0x7fb2a3ef0000 ret=0
kvm_set_user_memory Slot#65545 flags=0x0 gpa=0x100000 size=0xbff00000 ua=0x7fb2a3f00000 ret=0
kvm_set_user_memory Slot#6 flags=0x0 gpa=0xce000 size=0x0 ua=0x7fb2a3ece000 ret=0
kvm_set_user_memory Slot#7 flags=0x0 gpa=0xd0000 size=0x0 ua=0x7fb2a3ed0000 ret=0
kvm_set_user_memory Slot#6 flags=0x0 gpa=0xce000 size=0x1a000 ua=0x7fb2a3ece000 ret=0
kvm_set_user_memory Slot#7 flags=0x0 gpa=0xe8000 size=0x8000 ua=0x7fb2a3ee8000 ret=0
kvm_set_user_memory Slot#65542 flags=0x0 gpa=0xce000 size=0x0 ua=0x7fb2a3ece000 ret=0
kvm_set_user_memory Slot#65543 flags=0x0 gpa=0xd0000 size=0x0 ua=0x7fb2a3ed0000 ret=0
kvm_set_user_memory Slot#65542 flags=0x0 gpa=0xce000 size=0x1a000 ua=0x7fb2a3ece000 ret=0
kvm_set_user_memory Slot#65543 flags=0x0 gpa=0xe8000 size=0x8000 ua=0x7fb2a3ee8000 ret=0
kvm_set_user_memory Slot#4 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#65538 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#4 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#65538 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#4 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#65538 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#4 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#65538 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#4 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#65538 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#4 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#65538 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#4 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#65538 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#4 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#65538 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#4 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#65538 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#4 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#65538 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#4 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#65538 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#4 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#65538 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#4 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#65538 flags=0x0 gpa=0xfd000000 size=0x0 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#4 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7fb212000000 ret=0
kvm_set_user_memory Slot#65538 flags=0x1 gpa=0xfd000000 size=0x1000000 ua=0x7fb212000000 ret=0
```

## 为什么多次调用
至少 0x940000000 的大小是不会被调用多次，但是 4G 的部分会多次调用。

分析下 4G 下物理内存的:
1. 正经的初始化
```txt
#0  martins3 () at ../accel/kvm/kvm-all.c:290
#1  kvm_set_user_memory_region (slot=0x7fffe31b4178, new=new@entry=true, kml=<optimized out>, kml=<optimized out>) at ../accel/kvm/kvm-all.c:317
#2  0x0000555555c4d6d7 in kvm_set_phys_mem (kml=kml@entry=0x5555569a8690, section=section@entry=0x555556f8c700, add=<optimized out>, add@entry=true) at ../accel/kvm/kvm-all.c:1397
#3  0x0000555555c4dafc in kvm_region_commit (listener=0x5555569a8690) at ../accel/kvm/kvm-all.c:1544
#4  0x0000555555bc352e in memory_region_transaction_commit () at ../softmmu/memory.c:1109
#5  0x00005555559834bd in i440fx_update_memory_mappings (d=d@entry=0x555556c263a0) at ../hw/pci-host/i440fx.c:81
#6  0x000055555598413d in i440fx_init (pci_type=pci_type@entry=0x555555eae42e "i440FX", dev=dev@entry=0x555556ad1ea0, address_space_mem=address_space_mem@entry=0x55555677b2e0, address_space_io=address_space_io@entry=0x555556763000, ram_size=<optimized ou
t>, below_4g_mem_size=<optimized out>, above_4g_mem_size=39728447488, pci_address_space=0x555556990750, ram_memory=0x5555568f0ee0) at ../hw/pci-host/i440fx.c:308
#7  0x0000555555abebe7 in pc_init1 (machine=0x555556923d40, pci_type=0x555555eae42e "i440FX", host_type=0x555555eae44d "i440FX-pcihost") at ../hw/i386/pc_piix.c:227
#8  0x00005555558c1a1c in machine_run_board_init (machine=0x555556923d40, mem_path=<optimized out>, errp=<optimized out>) at ../hw/core/machine.c:1407
#9  0x0000555555a1e626 in qemu_init_board () at ../softmmu/vl.c:2512
#10 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2608
#11 0x0000555555a2219d in qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2603
#12 qemu_init (argc=<optimized out>, argv=<optimized out>) at ../softmmu/vl.c:3611
#13 0x0000555555843069 in main (argc=<optimized out>, argv=<optimized out>) at ../softmmu/main.c:47
```
2. listener，是不是每一个注册的都会调用一次？
```txt
#0  martins3 () at ../accel/kvm/kvm-all.c:290
#1  kvm_set_user_memory_region (slot=0x7ff54b7c00e8, new=new@entry=true, kml=<optimized out>, kml=<optimized out>) at ../accel/kvm/kvm-all.c:317
#2  0x0000555555c4d6d7 in kvm_set_phys_mem (kml=kml@entry=0x555556623c00 <smram_listener>, section=section@entry=0x555556c25330, add=<optimized out>, add@entry=true) at ../accel/kvm/kvm-all.c:1397
#3  0x0000555555c4dafc in kvm_region_commit (listener=0x555556623c00 <smram_listener>) at ../accel/kvm/kvm-all.c:1544
#4  0x0000555555bc6008 in listener_add_address_space (as=<optimized out>, listener=0x555556623c00 <smram_listener>) at ../softmmu/memory.c:2995
#5  memory_listener_register (listener=0x555556623c00 <smram_listener>, as=<optimized out>) at ../softmmu/memory.c:3058
#6  0x0000555555c4a2f8 in kvm_memory_listener_register (s=0x5555569a75d0, kml=0x555556623c00 <smram_listener>, as=0x555556623ba0 <smram_address_space>, as_id=1, name=0x555555f02eaa "kvm-smram") at ../accel/kvm/kvm-all.c:1714
#7  0x0000555555dd1bc7 in notifier_list_notify (list=list@entry=0x555556611110 <machine_init_done_notifiers>, data=data@entry=0x0) at ../util/notify.c:39
#8  0x00005555558c224b in qdev_machine_creation_done () at ../hw/core/machine.c:1455
#9  0x0000555555a1e7b3 in qemu_machine_creation_done () at ../softmmu/vl.c:2581
#10 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2610
#11 0x0000555555a2219d in qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2603
#12 qemu_init (argc=<optimized out>, argv=<optimized out>) at ../softmmu/vl.c:3611
#13 0x0000555555843069 in main (argc=<optimized out>, argv=<optimized out>) at ../softmmu/main.c:47
```
3. 我也不知道为什么是这个样子了？来自 guest 的行为导致需要注册一次。
```txt
#0  martins3 () at ../accel/kvm/kvm-all.c:290
#1  kvm_set_user_memory_region (slot=0x7fffe31b4298, new=new@entry=true, kml=<optimized out>, kml=<optimized out>) at ../accel/kvm/kvm-all.c:317
#2  0x0000555555c4d6d7 in kvm_set_phys_mem (kml=kml@entry=0x5555569a8690, section=section@entry=0x7ff5d4887290, add=<optimized out>, add@entry=true) at ../accel/kvm/kvm-all.c:1397
#3  0x0000555555c4dafc in kvm_region_commit (listener=0x5555569a8690) at ../accel/kvm/kvm-all.c:1544
#4  0x0000555555bc352e in memory_region_transaction_commit () at ../softmmu/memory.c:1109
#5  0x0000555555bc0310 in memory_region_write_accessor (mr=mr@entry=0x555556ad22e0, addr=0, value=value@entry=0x7ff5e2608498, size=size@entry=4, shift=<optimized out>, mask=mask@entry=4294967295, attrs=...) at ../softmmu/memory.c:493
#6  0x0000555555bbdaf6 in access_with_adjusted_size (addr=addr@entry=0, value=value@entry=0x7ff5e2608498, size=size@entry=4, access_size_min=<optimized out>, access_size_max=<optimized out>, access_fn=0x555555bc0290 <memory_region_write_accessor>, mr=0x555556ad22e0, attrs=...) at ../softmmu/memory.c:555
#7  0x0000555555bc1dba in memory_region_dispatch_write (mr=mr@entry=0x555556ad22e0, addr=0, data=<optimized out>, op=<optimized out>, attrs=attrs@entry=...) at ../softmmu/memory.c:1522
#8  0x0000555555bc8fe0 in flatview_write_continue (fv=fv@entry=0x7ff5d4832cf0, addr=addr@entry=3324, attrs=..., attrs@entry=..., ptr=ptr@entry=0x7ffff4bef000, len=len@entry=4, addr1=<optimized out>, l=<optimized out>, mr=0x555556ad22e0) at /home/martins3/core/qemu/include/qemu/host-utils.h:165
#9  0x0000555555bc92a0 in flatview_write (fv=0x7ff5d4832cf0, addr=addr@entry=3324, attrs=attrs@entry=..., buf=buf@entry=0x7ffff4bef000, len=len@entry=4) at ../softmmu/physmem.c:2677
#10 0x0000555555bcc4f9 in address_space_write (len=4, buf=0x7ffff4bef000, attrs=..., addr=3324, as=0x5555566264e0 <address_space_io>) at ../softmmu/physmem.c:2773
#11 address_space_rw (as=0x5555566264e0 <address_space_io>, addr=addr@entry=3324, attrs=attrs@entry=..., buf=0x7ffff4bef000, len=len@entry=4, is_write=is_write@entry=true) at ../softmmu/physmem.c:2783
#12 0x0000555555c4f04b in kvm_handle_io (count=1, size=4, direction=<optimized out>, data=<optimized out>, attrs=..., port=3324) at ../accel/kvm/kvm-all.c:2727
#13 kvm_cpu_exec (cpu=cpu@entry=0x5555569ada90) at ../accel/kvm/kvm-all.c:2978
#14 0x0000555555c504dd in kvm_vcpu_thread_fn (arg=arg@entry=0x5555569ada90) at ../accel/kvm/kvm-accel-ops.c:51
#15 0x0000555555dcc259 in qemu_thread_start (args=<optimized out>) at ../util/qemu-thread-posix.c:512
#16 0x00007ffff6688e86 in start_thread () from /nix/store/2j8jqmnd9l7plihhf713yf291c9vyqjm-glibc-2.35-224/lib/libc.so.6
#17 0x00007ffff670fd70 in clone3 () from /nix/store/2j8jqmnd9l7plihhf713yf291c9vyqjm-glibc-2.35-224/lib/libc.so.6
```




大多数是因为 pci 刷新了地址了:
```txt
#1  kvm_set_user_memory_region (slot=0x7fffe31b4130, new=new@entry=true, kml=<optimized out>, kml=<optimized out>) at ../accel/kvm/kvm-all.c:317
#2  0x0000555555c4d6d7 in kvm_set_phys_mem (kml=kml@entry=0x5555569a8690, section=section@entry=0x7ff5d4307910, add=<optimized out>, add@entry=true) at ../accel/kvm/kvm-all.c:1397
#3  0x0000555555c4dafc in kvm_region_commit (listener=0x5555569a8690) at ../accel/kvm/kvm-all.c:1544
#4  0x0000555555bc352e in memory_region_transaction_commit () at ../softmmu/memory.c:1109
#5  0x0000555555971626 in pci_update_mappings (d=d@entry=0x55555714d3c0) at ../hw/pci/pci.c:1502
#6  0x0000555555971e64 in pci_default_write_config (d=0x55555714d3c0, addr=4, val_in=259, l=2) at ../hw/pci/pci.c:1562
#7  0x0000555555bc0310 in memory_region_write_accessor (mr=mr@entry=0x555556ad22e0, addr=0, value=value@entry=0x7ff5e2608498, size=size@entry=2, shift=<optimized out>, mask=mask@entry=65535, attrs=...) at ../softmmu/memory.c:493
#8  0x0000555555bbdaf6 in access_with_adjusted_size (addr=addr@entry=0, value=value@entry=0x7ff5e2608498, size=size@entry=2, access_size_min=<optimized out>, access_size_max=<optimized out>, access_fn=0x555555bc0290 <memory_region_write_accessor>, mr=0x555556ad22e0, attrs=...) at ../softmmu/memory.c:555
#9  0x0000555555bc1dba in memory_region_dispatch_write (mr=mr@entry=0x555556ad22e0, addr=0, data=<optimized out>, op=<optimized out>, attrs=attrs@entry=...) at ../softmmu/memory.c:1522
#10 0x0000555555bc8fe0 in flatview_write_continue (fv=fv@entry=0x7ff5d42bec30, addr=addr@entry=3324, attrs=..., attrs@entry=..., ptr=ptr@entry=0x7ffff4bef000, len=len@entry=2, addr1=<optimized out>, l=<optimized out>, mr=0x555556ad22e0) at /home/martins3/core/qemu/include/qemu/host-utils.h:165
#11 0x0000555555bc92a0 in flatview_write (fv=0x7ff5d42bec30, addr=addr@entry=3324, attrs=attrs@entry=..., buf=buf@entry=0x7ffff4bef000, len=len@entry=2) at ../softmmu/physmem.c:2677
#12 0x0000555555bcc4f9 in address_space_write (len=2, buf=0x7ffff4bef000, attrs=..., addr=3324, as=0x5555566264e0 <address_space_io>) at ../softmmu/physmem.c:2773
#13 address_space_rw (as=0x5555566264e0 <address_space_io>, addr=addr@entry=3324, attrs=attrs@entry=..., buf=0x7ffff4bef000, len=len@entry=2, is_write=is_write@entry=true) at ../softmmu/physmem.c:2783
#14 0x0000555555c4f04b in kvm_handle_io (count=1, size=2, direction=<optimized out>, data=<optimized out>, attrs=..., port=3324) at ../accel/kvm/kvm-all.c:2727
#15 kvm_cpu_exec (cpu=cpu@entry=0x5555569ada90) at ../accel/kvm/kvm-all.c:2978
#16 0x0000555555c504dd in kvm_vcpu_thread_fn (arg=arg@entry=0x5555569ada90) at ../accel/kvm/kvm-accel-ops.c:51
#17 0x0000555555dcc259 in qemu_thread_start (args=<optimized out>) at ../util/qemu-thread-posix.c:512
#18 0x00007ffff6688e86 in start_thread () from /nix/store/2j8jqmnd9l7plihhf713yf291c9vyqjm-glibc-2.35-224/lib/libc.so.6
#19 0x00007ffff670fd70 in clone3 () from /nix/store/2j8jqmnd9l7plihhf713yf291c9vyqjm-glibc-2.35-224/lib/libc.so.6
```

## 这样多次调用，真的好吗？
- KVM_SET_USER_MEMORY_REGION 的触发的内容是什么？

## 为什么 smm 需要导致增加这么多的 KVM_SET_USER_MEMORY_REGION

## smm : arch/x86/kvm/smm.c

## register_smram_listene

## 这里的东西看下
docs/qemu/bios-memory.md

## 没看懂
https://github.com/tianocore/tianocore.github.io/wiki/Testing-SMM-with-QEMU,-KVM-and-libvirt

http://events17.linuxfoundation.org/sites/events/files/slides/kvmforum15-smm.pdf

## 这么说，没有 SMM ，连关机也是不正常的吗?
https://wiki.osdev.org/System_Management_Mode

> Theoretically, this means that SMM is not nearly as critical on a modern system as on a pre-ACPI system.

测试了一下，qemu 没有打开 smm ，正常关机的。

https://en.wikipedia.org/wiki/System_Management_Mode

> However, BIOS manufacturers and OEMs have relied on SMM for newer functionality like Advanced Configuration and Power Interface (ACPI)

其实，有点怀疑，SMM 真的是 ACPI 需要的，因为 ACPI 中断本来就是

## 可以通过这个控制风扇

drivers/hwmon/dell-smm-hwmon.c

```c
/*
 * Call the System Management Mode BIOS. Code provided by Jonathan Buzzard.
 */
static int i8k_smm_func(void *par)
{
	struct smm_regs *regs = par;
	unsigned char carry;

	/* SMM requires CPU 0 */
	if (smp_processor_id() != 0)
		return -EBUSY;

	asm volatile("out %%al,$0xb2\n\t"
		     "out %%al,$0x84\n\t"
		     "setc %0\n"
		     : "=mr" (carry),
		       "+a" (regs->eax),
		       "+b" (regs->ebx),
		       "+c" (regs->ecx),
		       "+d" (regs->edx),
		       "+S" (regs->esi),
		       "+D" (regs->edi));

	if (carry)
		return -EINVAL;

	return 0;
}
```

```txt
config SENSORS_DELL_SMM
	tristate "Dell laptop SMM BIOS hwmon driver"
	depends on ACPI_WMI
	depends on X86
	imply THERMAL
	help
	  This hwmon driver adds support for reporting temperature of different
	  sensors and controls the fans on Dell laptops via System Management
	  Mode provided by Dell BIOS.

	  When option I8K is also enabled this driver provides legacy /proc/i8k
	  userspace interface for i8kutils package.
```

## 找到 SMM 的源码

可以，在 seabios 中有 src/fw/smm.c

但是似乎也没有什么实质的内容

### 分析下 SMM 问题
1. smm ，aarch64 和 risc-v 有这个东西吗?

- https://tandasat.github.io/blog/2024/02/29/ISRD.html : 高级
- https://opensecuritytraining.info/IntroBIOS_files/Day1_07_Advanced%20x86%20-%20BIOS%20and%20SMM%20Internals%20-%20SMM.pdf


https://casualhacking.io/blog/2019/12/3/using-optionrom-to-overwrite-smmsmi-handlers-in-qemu

https://github.com/theopolis/uefi-firmware-parser

- arm 环境中如何模拟更加高级的 el 模型?
  - 需要类似 kvm 对于 smm 的支持吗

## 回答
- ACPI 需要使用 SMM 吗？
- ACPI 中断是什么中断？

## 之后的代办
- [ ] 将 UEFI 的代码合并一下
- [ ] 阅读 intel sdm
- [ ] kvm 实现
- [ ] kernel module 看看如何实现

## deepkseek : 找到 kvm forum 上介绍 SMM 的工作

KVM Forum 2015 - "Securing Secure Boot: System Management Mode in KVM" by Paolo Bonzini
Testing SMM with QEMU, KVM and libvirt

## System Management Mode deep dive: How SMM isolation hardens the platform
https://www.microsoft.com/en-us/security/blog/2020/11/12/system-management-mode-deep-dive-how-smm-isolation-hardens-the-platform/

> Windows 10 achieves this by leveraging a hardware-based root of trust that ensures unauthorized code like
Unified Extensible Firmware Interface (UEFI) malware cannot take root before the Windows bootloader launches.

两个问题，到底是什么?

思考一下，为什么打开了 secure boot ，结果 grub 无法?

那么如何攻击，如果没有 secure boot 的话?

## kvm 中，SMM 只有 secure boot 的作用

```txt
config KVM_SMM
	bool "System Management Mode emulation"
	default y
	depends on KVM
	help
	  Provides support for KVM to emulate System Management Mode (SMM)
	  in virtual machines.  This can be used by the virtual machine
	  firmware to implement UEFI secure boot.

	  If unsure, say Y.
```

### piix4_pm_realize 中
acpi_pm_tmr_timer

```txt
    acpi_pm_tmr_init(&s->ar, pm_tmr_timer, &s->io);
```
- pm_tmr_timer
  - acpi_update_sci

```c
struct PIIX4PMState {
    // ...
    qemu_irq irq;
    qemu_irq smi_irq;

```

```c
static void piix4_pm_init(Object *obj)
{
    PIIX4PMState *s = PIIX4_PM(obj);

    qdev_init_gpio_out(DEVICE(obj), &s->irq, 1);
    qdev_init_gpio_out_named(DEVICE(obj), &s->smi_irq, "smi-irq", 1);
}
```
所以看上去就是两个

```txt
  9:          0          0          0          0  IO-APIC   9-fasteoi   acpi
```

不过没有进一步调查了，但是 gpe 也是用的这个中断，所以不是 smi ，
```c
static void piix4_send_gpe(AcpiDeviceIf *adev, AcpiEventStatusBits ev)
{
    PIIX4PMState *s = PIIX4_PM(adev);

    acpi_send_gpe_event(&s->ar, s->irq, ev);
}
```
同时检查 smi_irq 的使用地方，不太可能有 qemu 的使用。

## 可以用 lenovo 的机器调查一下，Fn Q 调整机器频率是不是通过 SMI 实现的

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
