# xhci ohci 之类都是什么东西

https://en.wikipedia.org/wiki/Host_controller_interface_(USB,_Firewire)

linux/drivers/usb/host/ 下罗列各种 hci ，目前重要的 xhci

## ohci
- usb 键盘中断流程
```txt
[   75.597619] [<900000000020866c>] show_stack+0x2c/0x100
[   75.597621] [<9000000000ec39c8>] dump_stack+0x90/0xc0
[   75.597624] [<9000000000c4b1b0>] input_event+0x30/0xc8
[   75.597626] [<9000000000ca3ee4>] hidinput_report_event+0x44/0x68
[   75.597628] [<9000000000ca1e30>] hid_report_raw_event+0x230/0x470
[   75.597631] [<9000000000ca21a4>] hid_input_report+0x134/0x1b0
[   75.597632] [<9000000000cb07ac>] hid_irq_in+0x9c/0x280
[   75.597634] [<9000000000be9cf0>] __usb_hcd_giveback_urb+0xa0/0x120
[   75.597636] [<9000000000c23a7c>] finish_urb+0xac/0x1c0
[   75.597638] [<9000000000c24b50>] ohci_work.part.8+0x218/0x550 [   75.597640] [<9000000000c27f98>] ohci_irq+0x108/0x320
[   75.597642] [<9000000000be96e8>] usb_hcd_irq+0x28/0x40
[   75.597644] [<9000000000296430>] __handle_irq_event_percpu+0x70/0x1b8
[   75.597645] [<9000000000296598>] handle_irq_event_percpu+0x20/0x88
[   75.597647] [<9000000000296644>] handle_irq_event+0x44/0xa8
[   75.597648] [<900000000029abfc>] handle_level_irq+0xdc/0x188
[   75.597651] [<90000000002952a4>] generic_handle_irq+0x24/0x40
[   75.597652] [<900000000081dc50>] extioi_irq_dispatch+0x178/0x210
[   75.597654] [<90000000002952a4>] generic_handle_irq+0x24/0x40
[   75.597656] [<9000000000ee4eb8>] do_IRQ+0x18/0x28
[   75.597658] [<9000000000203ffc>] except_vec_vi_end+0x94/0xb8
[   75.597660] [<9000000000203e80>] __cpu_wait+0x20/0x24
[   75.597662] [<900000000020fa90>] calculate_cpu_foreign_map+0x148/0x180
```

## xhci

对于 U 盘 fio 就是这个结果:
```txt
@[
    xhci_irq+1
    __handle_irq_event_percpu+70
    handle_irq_event+58
    handle_edge_irq+177
    __common_interrupt+105
    common_interrupt+179
    asm_common_interrupt+34
    cpuidle_enter_state+222
    cpuidle_enter+41
    do_idle+492
    cpu_startup_entry+25
    start_secondary+271
    secondary_startup_64_no_verify+224
]: 1759
```

## [ ]  从 scsi 到 usb 是如何进行的

两个超级类似的 patch :
```diff
History:        #0
Commit:         04d21f7219acec66751f5512aa8a69f528c5b36a
Author:         Mathias Nyman <mathias.nyman@linux.intel.com>
Committer:      Greg Kroah-Hartman <gregkh@linuxfoundation.org>
Author Date:    Fri 29 Jan 2021 09:00:26 PM CST
Committer Date: Fri 29 Jan 2021 09:16:50 PM CST

xhci: prevent a theoretical endless loop while preparing rings.

xhci driver links together segments in a ring buffer by turning the last
TRB of a segment into a link TRB, pointing to the beginning of
the next segment.

If the first TRB of every segment for some unknown reason is a link TRB
pointing to the next segment, then prepare_ring() loops indefinitely.
This isn't something the xhci driver would do.
xHC hardware has access to these rings, it sholdn't be writing link
TRBs either, but with broken xHC hardware this could in theory be
possible.

Signed-off-by: Mathias Nyman <mathias.nyman@linux.intel.com>
Link: https://lore.kernel.org/r/20210129130044.206855-10-mathias.nyman@linux.intel.com
Signed-off-by: Greg Kroah-Hartman <gregkh@linuxfoundation.org>
```

```diff
History:        #0
Commit:         c716e8a5fada15df495aa4891f4f22e51b95de11
Author:         Mathias Nyman <mathias.nyman@linux.intel.com>
Committer:      Greg Kroah-Hartman <gregkh@linuxfoundation.org>
Author Date:    Fri 29 Jan 2021 09:00:30 PM CST
Committer Date: Fri 29 Jan 2021 09:16:50 PM CST

xhci: Check link TRBs when updating ring enqueue and dequeue pointers.

xhci driver relies on link TRBs existing in the correct places in TRB
ring buffers shared with the host controller.
The controller should not modify these link TRBs, but in theory a faulty
xHC could do it.

Add some basic sanity checks to avoid infinite loops in interrupt handler,
or accessing unallocated memory outside a ring segment due to missing or
misplaced link TRBs.

Signed-off-by: Mathias Nyman <mathias.nyman@linux.intel.com>
Link: https://lore.kernel.org/r/20210129130044.206855-14-mathias.nyman@linux.intel.com
Signed-off-by: Greg Kroah-Hartman <gregkh@linuxfoundation.org>
```

没有 backport 这个代码 : https://gitee.com/openeuler/kernel/blob/openEuler-22.03-LTS/drivers/usb/host/xhci-ring.c#L156

https://www.cs.cmu.edu/~412/lectures/L05_xHCI.pdf

TBR :

当从将 USB controller 去掉的时候:
```txt
09:00.0 USB controller: Advanced Micro Devices, Inc. [AMD] Device 15b8
```
```sh
echo 0000:09:00.0 | sudo tee /sys/bus/pci/devices/0000:09:00.0/driver/unbind
```
```txt
[ 3105.455424] xhci_hcd 0000:09:00.0: remove, state 4 [ 3105.455429] usb usb6: USB disconnect, device number 1
[ 3105.455528] xhci_hcd 0000:09:00.0: USB bus 6 deregistered
[ 3105.455538] xhci_hcd 0000:09:00.0: remove, state 1
[ 3105.455540] usb usb5: USB disconnect, device number 1
[ 3105.455541] usb 5-1: USB disconnect, device number 2
[ 3105.455541] usb 5-1.1: USB disconnect, device number 3
[ 3105.457767] usb 5-1.2: USB disconnect, device number 4
[ 3105.651330] xhci_hcd 0000:09:00.0: USB bus 5 deregistered
```
