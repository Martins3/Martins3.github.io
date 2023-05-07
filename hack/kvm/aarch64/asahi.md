# hacking asahi linux

如何支持安装 ftrace

## 好家伙，中断控制器都不是 GIC

- [ ] 并且 kvm guest ptimer 和 kvm guest vtimer

```txt
🧀  cat /proc/interrupts
           CPU0       CPU1       CPU2       CPU3       CPU4       CPU5       CPU6       CPU7
 33:          0          0          0          0          0          0          0          0       KVM   0 Level     kvm guest ptimer
 34:          0          0          0          0          0          0          0          0       KVM   1 Level     kvm guest vtimer
 35:    7755597    9655642    8566153    7032013   14442467   14024790   24046855   22478076   AIC-FIQ   0 Level     arch_timer
 37:          0          0          0          0          0          0          0          0      AIC2 66248 Level     206408000.mbox-recv
 38:          0          0          0          0          0          0          0          0      AIC2 66245 Level     206408000.mbox-send
 39:          0          0          0          0          0          0          0          0      AIC2 66074 Level     231c08000.mbox-recv
 40:          0          0          0          0          0          0          0          0      AIC2 66071 Level     231c08000.mbox-send
 41:       3111       1683        954        813      62373      15332       3660       2035      AIC2 66038 Level     23e408000.mbox-recv
 42:          0          0          0          0          0          0          0          0      AIC2 66035 Level     23e408000.mbox-send
 43:         14          3          2          4          1          0          2          3      AIC2 66403 Level     24e408000.mbox-recv
 44:          0          0          0          0          0          0          0          0      AIC2 66400 Level     24e408000.mbox-send
 45:          0          1          0          0          3          2         14         12      AIC2 66256 Level     277408000.mbox-recv
 46:          0          0          0          0          0          0          0          0      AIC2 66253 Level     277408000.mbox-send
 49:          0          0          0          0          0          0          0          0   AIC-FIQ   4 Level     arm-pmu
 50:          0          0          0          0          0          0          0          0   AIC-FIQ   5 Level     arm-pmu
 51:     157167      63352      32229      18788      54949      26252      25984      21969      AIC2 66260 Level     nvme-apple
 52:          0          0          0          0          0          0          0          0      AIC2 66089 Level     apple-dart fault handler, apple-dart fault handler
 53:          0          0          0          0          0          0          0          0      AIC2 66305 Level     apple-dart fault handler
 54:          0          0          0          0          0          0          0          0      AIC2 66384 Level     apple-dart fault handler
 55:          0          0          0          0          0          0          0          0      AIC2 66571 Level     apple-dart fault handler, apple-dart fault handler
 56:          0          0          0          0          0          0          0          0      AIC2 66652 Level     apple-dart fault handler, apple-dart fault handler
 57:          0          0          0          0          0          0          0          0      AIC2 66318 Level     apple-dart fault handler
 73:          0          0          0          0          0          0          0          0      AIC2 66301 Level     pasemi_apple_i2c
 74:          0          0          0          0          0          0          0          0      AIC2 66285 Level     235104000.spi
 75:         32          8         10          7         21         11         11         22      AIC2 66297 Level     pasemi_apple_i2c
 76:          0          0          0          0          0          0          0          0      AIC2 66298 Level     pasemi_apple_i2c
 77:          0          0          0          0          0          0          0          0      AIC2 66299 Level     pasemi_apple_i2c
 78:         65         31          8          4          4          6          4          6      AIC2 66300 Level     pasemi_apple_i2c
 93:          6          1          1          2          4          2          1          1  Apple-GPIO   8 Level     1-0038, 1-003f
 94:          0          0          0          0          0          0          0          0  dockchannel-irqc   2 Edge      apple-dockchannel-tx
 95:       2912        890        785        822       6962       2533       3463        412  dockchannel-irqc   3 Edge      apple-dockchannel-rx
 97:          0          0          0          0          0          1          0          0      PCIe  12 Edge      Link up
 98:          0          0          0          0          0          0          0          0      PCIe  14 Edge      Link down
100:          0          0          0          0          0          0          0          0  PCIe MSI   0 Edge      PCIe PME, aerdrv
101:          0          0          0          0          0          0          0          0      AIC2 66296 Level     238200000.dma-controller
102:          0          1          0          0          0          0          0          1  Apple-GPIO 149 Level     cs42l84
104:         39          7         52          6         25         18         43         28  PCIe MSI 526336 Edge      bcm4377
105:      69224      34563      14650      11604     373757     124035      78794      60963  PCIe MSI 524288 Edge      brcmf_pcie_intr
IPI0:    106088     116924     126718     117934     404619     382391     310192     242062       Rescheduling interrupts
IPI1:   6531937    5465585    4908987    4327894    3303307    3866256    3334291    2757015       Function call interrupts
IPI2:         0          0          0          0          0          0          0          0       CPU stop interrupts
IPI3:         0          0          0          0          0          0          0          0       CPU stop (for crash dump) interrupts
IPI4:         0          0          0          0          0          0          0          0       Timer broadcast interrupts
IPI5:         0          0          0          0          0          0          0          0       IRQ work interrupts
IPI6:         0          0          0          0          0          0          0          0       CPU wake-up interrupts
Err:          0
```
