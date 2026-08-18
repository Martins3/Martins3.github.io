## qemu gpu ，这个的确是一个好玩的东西
<!-- d2b28f35-4de7-45bc-918d-6385d49092c4 -->

什么时候需要使用 nodisplay

```txt
# 多谢使用 novnc ，都已经忘记什么时候需要 disable vnc 了
if check_option nodisplay; then
	arg_display+=" -display none"
	return
fi
```
必须配置 vnc ，如果不去配置 display none 的话

```sh
	# 参考这个吧:
	# https://www.kraxel.org/blog/2019/09/display-devices-in-qemu/
	#
	# aarch64 不支持 VGA ，需要配置上 modern 一点显示设备和键盘设备
	#
	# 所以在 x86 中可以看到如下两个设备:
	# 00:02.0 VGA compatible controller: Device 1234:1111 (rev 02)
	# 00:10.0 Display controller: Red Hat, Inc. Virtio GPU (rev 01)
	#
	# arg_display+="-device virtio-gpu-pci " # 这个和 windows 配合有问题，开机后直接黑屏
	# arg_display+="-device virtio-gpu-gl-pci" # 需要 QEMU 支持 openGL 重新编译才可以，TODO 到时候尝试下

		# 似乎，当有两个 GPU ，那么有时候 windows 直接黑屏
		# 可能实现的 GPU ，可能显示的不是正确的屏幕
		# gpu=" -device cirrus-vga "
		# gpu=" -device virtio-gpu-pci "

	# 如果增加了 -device virtio-vga 的设备，那么虚拟机可以观察到:
	# 将 00:02.0 VGA compatible controller: Device 1234:1111 (rev 02)
	# 替换为: 00:11.0 VGA compatible controller: Virtio: Virtio GPU (rev 01)
	# 看来 qemu 是使用了一些技巧的
	#
	# 此外，virtio vga 也是 aarch64 特有的
	#
	# virtio-vga 也是需要 kernel 特殊支持的，TODO 但是现在不知道打开 kernel 的哪一部分
	# if [[ $ARCH == x86_64 ]]; then
	# 	arg_display+="-device virtio-vga "
	# fi

	# TODO 所以，可能让 virtio GPU 作为 x86 环境中默认的显示吗?

	# TODO 不知道为什么 -vga virtio 鼠标是不能用的
	# arg_display+=" -vga virtio"
	# arg_display+=" -vga std"
```

## 图形显示
之前在使用 QEMU 安装 nixos 的时候，发现有时候 alacirtty 不能全屏，
似乎如果将 -vga virtio 修改为 -vga std 就可以解决
