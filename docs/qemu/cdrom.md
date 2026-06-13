# 如何给 qemu 配置 cdrom

浪费了很多时间，还是记录一下吧

正确的方法:
```txt
	-device scsi-cd,bus=scsi1.0,channel=0,scsi-id=20,lun=1,drive=cd1 \
	-drive file=/home/martins3/data/hack/iso/Fedora-Server-dvd-x86_64-43-1.6.iso,format=raw,if=none,id=cd2,readonly=on \
```
这个可以被 edk2 / seabios 识别，没有数量限制


-machine pc 的时候，下面的 cdrom_counter 只能是 0 ，也就是最多仅仅支持一个 cdrom ，
而当 -machine q35 的时候，可以配置多个 cdrom_counter
```txt
	arg_cdrom+=" -device ide-cd,bus=ide.$cdrom_counter"
```

deepseek 对于 -device ide-cd,bus=ide.1,unit=1,drive=cd1: Can't create IDE unit 1, bus supports only 1 units 的解释:

 In traditional IDE controllers:
 Each IDE bus supports exactly 2 units (unit 0 and unit 1), often called "master" and "slave"
 The first IDE bus is ide.0
 The second IDE bus is ide.1

 使用 pc machine type 的时候，就是这样的:
 qemu-system-x86_64: -device ide-cd,bus=ide.0,unit=2,drive=cd0: Can't create IDE unit 2, bus supports only 2 units

所以，测试可以得到结论:
1. pc ide 仅仅支持一个 iso ，所以 unit 必须为 0
2. q35 支持两个, unit 可以 0 和 1
3. 最后还是用 ide.0 和 ide.1 就可以了

 pc 如果使用 ide.0 和 ide.1 ，那么 ide.1 上的 iso 无法识别，直接用
 -device ide-cd 参数，和使用 ide.0,unit=0 和 ide.0,unit=1 类似

 man qemu(1)
               You can connect a CDROM to the slave of ide0:
                 qemu-system-x86_64 -drive file=file,if=ide,index=1,media=cdrom

## 为什么会把 iso 启动搞那么复杂?
```sh
# 如果从 0 开始，会有这个错误:
# cdrom_counter=1
# qemu-system-x86_64: -drive file=/home/martins3/hack/iso/openEuler-24.03-LTS-x86_64-dvd.iso,format=raw,
# id=cd0,if=none,media=cdrom,index=0: drive with bus=0, unit=0 (index=0) exists
function add_cdrom() {

	# 这种奇怪的写法遇到的问题是:
	# windows 下，machine 为 pc 的时候，virtio-win 无法识别
	local file=$1
	local id=cd$cdrom_counter

	# 像是只能识别 IDE 的 rom 一样，先这样吧
	if [[ -z ${2:-} ]]; then
		arg_cdrom+=" -drive file=${1},format=raw,id=$id,if=none,media=cdrom,index=$cdrom_counter,readonly=on"
		# 如果让 cdrom 启动的更加早，添加 ,bootindex=1 ，这个时候，需要赶快去按屏幕，让机器从 CDROM 启动，
		# 不然会进入 UEFI shell 中。
		local index=",bootindex=2"
		if check_option install; then
			local index=",bootindex=1"
		fi
		arg_cdrom+=" -device scsi-cd,bus=scsi4.0,channel=0,scsi-id=20,lun=$cdrom_counter,drive=$id$index"
	else
		# q35 ide-cd unit=0 似乎都是需要仔细配置的
		arg_cdrom+=" -drive file=$1,format=raw,if=none,id=$id,readonly=on "
		# arg_cdrom+=" -device ide-cd,bus=ide.$cdrom_counter,unit=0,drive=$id "
		arg_cdrom+=" -device ide-cd,bus=ide.0,unit=0,drive=$id "

		# arg_cdrom+=" -drive file=${1},format=raw,id=$id,if=none,media=cdrom,index=$cdrom_counter,readonly=on"
		# arg_cdrom+=" -device scsi-cd,bus=scsi4.0,channel=0,scsi-id=20,lun=$cdrom_counter,drive=$id"
	fi
	cdrom_counter=$((cdrom_counter + 1))
}
```

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
