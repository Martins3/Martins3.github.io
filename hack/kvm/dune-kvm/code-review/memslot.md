# memslot

在 x86 下 mmap_base 和 start_stack 的结果
```
[ 3391.660900] tmux: client ---> 7faf10520000 7ffc10f26050
[ 3391.660901] nvim ---> 7f46f1c26000 7fff3e0ecde0
[ 3391.660902] python3 ---> 7fb22bf92000 7ffc414c22f0
[ 3391.660903] node ---> 7fa8d660d000 7ffcbc77f490
[ 3391.660904] python3 ---> 7fd2f11e7000 7ffd3b8d55c0
[ 3391.660906] ccls ---> 7ff47aa41000 7fff3f5532c0
[ 3391.660906] kworker/5:0 doesn't have mm
[ 3391.660907] zsh ---> 7f74d9c8b000 7ffc54de1ee0
[ 3391.660908] kworker/6:0 doesn't have mm
[ 3391.660909] kworker/u16:0 doesn't have mm
[ 3391.660910] zsh ---> 7f06275f5000 7ffff896ec00
[ 3391.660911] nvim ---> 7ffaa0bd7000 7fffd79bd540
[ 3391.660914] python3 ---> 7ff6bdc9e000 7ffe8a27f670
[ 3391.660915] node ---> 7fd2849b4000 7ffd85460400
[ 3391.660916] python3 ---> 7f49ff6ca000 7ffd189202d0
[ 3391.660916] kworker/u16:3 doesn't have mm
[ 3391.660917] zsh ---> 7fdf732cd000 7ffee2fdb8e0
[ 3391.660918] ssh ---> 7f11c8e92000 7ffd9132dcf0
[ 3391.660919] ssh-agent ---> 7ff9d6465000 7ffe2c6857a0
[ 3391.660920] zsh ---> 7f4f4e910000 7ffc7a4f4580
```

在 MIPS 下, 可以确定约束到 40 bit 了
```
[28562.984327] wnck-applet ---> fff7fe0000 fffb9c4e50
[28562.984330] clock-applet ---> fff7650000 fffbcbfa30
[28562.984332] notification-ar ---> fff6e74000 fffb889720
[28562.984335] softupd_applet ---> fff4ed8000 fffbb28ff0
[28562.984338] dbus-daemon ---> fff4c3c000 fffb9a5f40
[28562.984340] upowerd ---> fff6b34000 fffbb5dda0
[28562.984342] at-spi2-registr ---> fff54a8000 fffbf476f0
[28562.984346] gvfsd-trash ---> fff7820000 fffbe3f210
[28562.984349] fcitx ---> fff4e88000 fffbdb9af0
[28562.984351] dbus-daemon ---> fff4548000 fffba8d650
[28562.984354] fcitx-dbus-watc ---> fff4c2c000 fffb9718a0
[28562.984356] gvfsd-metadata ---> fff64d8000 fffbca74a0
[28562.984359] IsaHelpTray ---> fff75bc000 fffb8e9ba0
[28562.984361] bluetoothd ---> fff5aa0000 fffbd95120
[28562.984364] packagekitd ---> fff4de4000 fffb7f5170
[28562.984366] chromium-browse ---> fff5bfc000 fffb942f00
[28562.984369] chrome-sandbox ---> fff72ac000 fffbd815b0
[28562.984371] chromium-browse ---> fff5174000 fffbfa7070
[28562.984374] chromium-browse ---> fff5174000 fffbfa7070
[28562.984376] gcr-prompter ---> fff6858000 fffbc72ac0
```

- [ ] start_stack 和 mmap_base 的生长方向其实不用在乎

让 alloc_hva 的时候，size = 40, user 和 physical 都是 0

所以，内核会允许我们创建一个 1 << 40 的物理内存吗 ?
- [x] kvm 的 source code 检查
- [ ] TLB 支持吗 ?

## memslot 的创建
- kvm_vm_ioctl_set_memory_region
  - kvm_set_memory_region
    - `__kvm_set_memory_region`
      - 然后，就是各种检查
