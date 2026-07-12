# 如何替换 kernel

5. 将 initramfs 拷贝出来

进入到 guest 中:
```sh
scp initramfs-6.1.19-7.0.0.17.oe2303.x86_64.img martins3@10.0.2.2:/home/martins3/hack/vm/iso-name-initramfs.img
```

4. 修改 qde 修改 config

```sh
echo $HOME/data/linux-build > opt/kernel
```

6. 将 partuuid 或者 lvm 拷贝出来，似乎这个原理

方法 1 : 在 guest 中执行: cat /proc/cmdline
```txt
root=/dev/mapper/ao-root resume=/dev/mapper/ao-swap rd.lvm.lv=ao/root rd.lvm.lv=ao/swap
```
echo "...." > cmdline 中

方法 2 :
- blkid

## 网络配置

为什么总是需要创建一个 bridge 让 Linux 访问网络啊
- https://myme.no/posts/2021-11-25-nixos-home-assistant.html
- https://gist.github.com/extremecoders-re/e8fd8a67a515fee0c873dcafc81d811c

## 网络配置 vlan id

对应的网络配置为:
```txt
  <interface type='bridge'>
      <mac address='52:54:00:f5:11:87'/>
      <source bridge='ovsbr-u6dbvk5z4'/>
      <vlan>
        <tag id='0'/>
      </vlan>
      <virtualport type='openvswitch'>
        <parameters interfaceid='32be67b5-48de-4fe4-b7b9-ff541de6097f' profileid='c8a1e42d-e0f3-4d50-a190-53209a98f157'/>
      </virtualport>
      <model type='virtio'/>
      <driver name='vhost' queues='4' rx_queue_size='1024'/>
      <link state='up'/>
      <address type='pci' domain='0x0000' bus='0x00' slot='0x04' function='0x0'/>
    </interface>
```
-netdev tap,fds=48:49:50:51,id=hostnet0,vhost=on,vhostfds=52:53:54:55
-device virtio-net-pci,mq=on,vectors=10,rx_queue_size=1024,netdev=hostnet0,id=net0,mac=52:54:00:f5:11:87,bus=pci.0,addr=0x4

```txt
   <interface type='hostdev' managed='yes'>
      <mac address='52:54:00:6f:17:47'/>
      <source>
        <address type='pci' domain='0x0000' bus='0xa3' slot='0x00' function='0x4'/>
      </source>
      <vlan>
        <tag id='0'/>
      </vlan>
      <address type='pci' domain='0x0000' bus='0x00' slot='0x0c' function='0x0'/>
    </interface>
```
-device vfio-pci,host=0000:a3:00.4,id=hostdev0,bus=pci.0,addr=0xc

但是，如果配置是直通，就没有 vlan=id 了。
```txt
    <hostdev mode='subsystem' type='pci' managed='yes'>
      <source>
        <address domain='0x0000' bus='0xa3' slot='0x00' function='0x0'/>
      </source>
      <alias name='ua-93add05b-30fb-50c0-b8ca-a693e6ae2d23'/>
      <address type='pci' domain='0x0000' bus='0x00' slot='0x0c' function='0x0'/>
    </hostdev>
```

配置方法:
```txt
ovs-vsctl add-port br0 tap0 tag=100
```


## 如果 network wait on line 启动失败

那么 nmcli c show 看看，黄色的那个就是失败，给删掉就可以了


## 几个启动模式的说明
- vmtest : 调试细节 vmtest.md ，考虑的如何启动一个最简的 linux ，使用 qemu 启动
	- vmtest 需要专用的内核，由于没有配置 initrd (为什么没有 initrd 来着?)
- firecrasher : 用 firecrasher 启动
- nixos :
	- 更多的讨论在 code/qemu/nixos.md 中

## 如果完整的验证，其实也就是额外的验证 arm 和 windows 11 平台了

## 可以直接让一个盘是 ext4 格式的，
但是可以通过这种方法预设内容，然后使用 raw 格式启动？
```txt
		 mount -o loop /path/to/data /mnt
		 dd if=/dev/null of="${ext4_img}" bs=1M seek=1000
		 mkfs.ext4 -F "${ext4_img}"
```
但是这个没有启动盘啊

## 需要重新梳理一下 install_mode 才可以
install_mode=""
应该是指定了 install mode 之后，立刻写入配置文件，
之后，就不要用 install_mode 这个变量了

## 如果系统安装 debuginfo rpm ，那么就可以使用 crash 开始 live 分析当前内核

## arm windows

获取 arm windows 的 iso 不难

都有人走通过了，就不需要继续了:
- https://www.reddit.com/r/ARMWindows/comments/1gb6m9l/running_win11_arm64_on_x86_via_qemu/
- https://gist.github.com/Vogtinator/293c4f90c5e92838f7e72610725905fd

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
