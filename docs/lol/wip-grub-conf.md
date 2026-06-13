# 我靠，感觉配置文件巨鸡儿混乱

/etc/grub.d 目录下

```txt
/etc/grub.d
total 116K
drwx------.   2 root root 4.0K Nov 27  2022 .
drwxr-xr-x. 115 root root  12K Jun 16 11:07 ..
-rwxr-xr-x.   1 root root 9.5K Nov 27  2022 00_header
-rwxr-xr-x.   1 root root 1.1K Jul  5  2018 00_tuned
-rwxr-xr-x.   1 root root  232 Aug 30  2022 01_users
-rwxr-xr-x.   1 root root  19K Aug 30  2022 10_linux
-rwxr-xr-x.   1 root root  830 Aug 30  2022 10_reset_boot_success
-rwxr-xr-x.   1 root root  889 Aug 30  2022 12_menu_auto_hide
-rwxr-xr-x.   1 root root  407 Aug 30  2022 14_menu_show_once
-rwxr-xr-x.   1 root root  14K Aug 30  2022 20_linux_xen
-rwxr-xr-x.   1 root root 2.5K Aug 30  2022 20_ppc_terminfo
-rwxr-xr-x.   1 root root  11K Aug 30  2022 30_os-prober
-rwxr-xr-x.   1 root root 1.1K Aug 30  2022 30_uefi-firmware
-rwxr-xr-x.   1 root root  214 Aug 30  2022 40_custom
-rwxr-xr-x.   1 root root  215 Aug 30  2022 41_custom
-rw-r--r--.   1 root root  483 Aug 30  2022 README
```

## 这应该是结果
/boot/grub2/grubenv
/boot/efi/EFI/openEuler/grubenv


存在这个软链接:
```txt
/boot/grub2/grubenv -> ../efi/EFI/openEuler/grubenv
```

/etc/grub2-efi 之类的目录

1. 结论，直接有效，应该如何操作那个文件
