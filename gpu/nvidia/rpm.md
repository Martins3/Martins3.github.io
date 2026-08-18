## 基本操作
看看如何直接安装各种包吧

https://rpmfusion.org/Howto/NVIDIA

https://github.com/Comprehensive-Wall28/Nvidia-Fedora-Guide

```sh
sudo dnf install https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm https://mirrors.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm
sudo dnf install akmod-nvidia
sudo dnf install xorg-x11-drv-nvidia-cuda #for cuda and nvidia-smi
```
然后就可以了，可以开始 cuda 测试了

如果想要直接构建驱动，改如何操作，应该直接安装一个就可以了?


到底是什么驱动?
https://developer.nvidia.com/blog/nvidia-transitions-fully-towards-open-source-gpu-kernel-modules/

> For cutting-edge platforms such as NVIDIA Grace Hopper or NVIDIA Blackwell, you must use the open-source GPU kernel modules. The proprietary drivers are unsupported on these platforms.
>
> For newer GPUs from the Turing, Ampere, Ada Lovelace, or Hopper architectures, NVIDIA recommends switching to the open-source GPU kernel modules.
>
> For older GPUs from the Maxwell, Pascal, or Volta architectures, the open-source GPU kernel modules are not compatible with your platform. Continue to use the NVIDIA proprietary driver.

不过，最后是让 kimi 帮安装的，还需要解决 cuda 版本太高的问题。

只不过现在每次执行命令的时候，都会出现这些警告:

```txt
sudo yum upgrade

[sudo] password for martins3:
Updating and loading repositories:
Repositories loaded.
Problem 1: installed package xorg-x11-drv-nvidia-3:580.142-1.fc42.x86_64 conflicts with xorg-x11-drv-nvidia provided by nvidia-driver-3:590.48.01-1.fc42.x86_64 from cuda-fedora42-x86_64
  - package nvidia-settings-3:590.48.01-1.fc42.x86_64 from cuda-fedora42-x86_64 requires nvidia-driver(x86-64) = 3:590.48.01, but none of the providers can be installed
  - cannot install the best update candidate for package xorg-x11-drv-nvidia-3:580.142-1.fc42.x86_64
  - cannot install the best update candidate for package nvidia-settings-3:580.142-1.fc42.x86_64
 Problem 2: installed package xorg-x11-drv-nvidia-cuda-3:580.142-1.fc42.x86_64 requires nvidia-modprobe(x86-64) = 3:580.142, but none of the providers can be installed
  - cannot install both nvidia-modprobe-3:590.48.01-1.fc42.x86_64 from cuda-fedora42-x86_64 and nvidia-modprobe-3:580.142-1.fc42.x86_64 from @System
  - cannot install the best update candidate for package xorg-x11-drv-nvidia-cuda-3:580.142-1.fc42.x86_64
  - cannot install the best update candidate for package nvidia-modprobe-3:580.142-1.fc42.x86_64
 Problem 3: installed package xorg-x11-drv-nvidia-3:580.142-1.fc42.x86_64 requires (xorg-x11-drv-nvidia-cuda = 3:580.142-1.fc42 if cuda-toolkit), but none of the providers can be installed
  - installed package xorg-x11-drv-nvidia-cuda-3:580.142-1.fc42.x86_64 requires nvidia-persistenced(x86-64) = 3:580.142, but none of the providers can be installed
  - installed package xorg-x11-drv-nvidia-power-3:580.142-1.fc42.x86_64 requires xorg-x11-drv-nvidia(x86-64) = 3:580.142, but none of the providers can be installed
  - cannot install both nvidia-persistenced-3:590.48.01-1.fc42.x86_64 from cuda-fedora42-x86_64 and nvidia-persistenced-3:580.142-1.fc42.x86_64 from @System
  - cannot install the best update candidate for package xorg-x11-drv-nvidia-power-3:580.142-1.fc42.x86_64
  - cannot install the best update candidate for package nvidia-persistenced-3:580.142-1.fc42.x86_64
  - cannot install the best update candidate for package cuda-toolkit-13.1.1-1.x86_64
```

## TODO

nvidia-gpu-firmware-20250311-1.fc42.noarch 是做什么的，
这个占用了很多 initramfs 的空间而且还很大。
```txt
🧀    sudo dnf remove nvidia-gpu-firmware
[sudo] password for martins3:
Package                  Arch    Version                  Repository        Size
Removing:
 nvidia-gpu-firmware     noarch  20260221-1.fc42          updates      101.0 MiB
```

## nvidia 中 feodra 中的 rpm 包
<!-- 44b06dfd-7f01-4fb4-a6e5-d331b1585256 -->

```txt

nvidia-modprobe-580.95.05-1.fc42.x86_64
xorg-x11-drv-nvidia-cuda-libs-580.95.05-1.fc42.x86_64
nvidia-persistenced-580.95.05-1.fc42.x86_64
xorg-x11-drv-nvidia-libs-580.95.05-1.fc42.x86_64
xorg-x11-drv-nvidia-kmodsrc-580.95.05-1.fc42.x86_64
akmod-nvidia-580.95.05-1.fc42.x86_64
nvidia-settings-580.95.05-1.fc42.x86_64
xorg-x11-drv-nvidia-580.95.05-1.fc42.x86_64
xorg-x11-drv-nvidia-cuda-580.95.05-1.fc42.x86_64
xorg-x11-drv-nvidia-power-580.95.05-1.fc42.x86_64
kmod-nvidia-6.17.5-200.fc42.x86_64-580.95.05-1.fc42.x86_64
```

这就很奇怪了:
既然 nvidia-gpu-firmware-20250311-1.fc42.noarch 已经有 firmware ，
那么 GPU 驱动中还是驱动，这是为什么?

连 1060 都是用的 GPU 驱动，但是直通的时候有用这个吗?
```txt
martins3@localhost:~$ modinfo nvidia
filename:       /lib/modules/6.17.5-200.fc42.x86_64/extra/nvidia/nvidia.ko
alias:          char-major-195-*
version:        580.95.05
supported:      external
license:        NVIDIA
firmware:       nvidia/580.95.05/gsp_tu10x.bin
firmware:       nvidia/580.95.05/gsp_ga10x.bin
```

那么直通的时候使用的固件是来自于虚拟机中的，还是物理机中的?

各个包展开之后，结果如下:
```txt
-- nvidia-modprobe-580.142-1.fc42.x86_64 --
/usr/bin/nvidia-modprobe
/usr/share/licenses/nvidia-modprobe
/usr/share/licenses/nvidia-modprobe/COPYING
/usr/share/man/man1/nvidia-modprobe.1.gz

-- xorg-x11-drv-nvidia-cuda-libs-580.142-1.fc42.x86_64 --
/etc/OpenCL/vendors/nvidia.icd
/usr/lib/modprobe.d/nvidia-uvm.conf
/usr/lib64/libcuda.so
/usr/lib64/libcuda.so.1
/usr/lib64/libcuda.so.580.142
/usr/lib64/libcudadebugger.so.1
/usr/lib64/libcudadebugger.so.580.142
/usr/lib64/libnvcuvid.so
/usr/lib64/libnvcuvid.so.1
/usr/lib64/libnvcuvid.so.580.142
/usr/lib64/libnvidia-encode.so
/usr/lib64/libnvidia-encode.so.1
/usr/lib64/libnvidia-encode.so.580.142
/usr/lib64/libnvidia-ml.so
/usr/lib64/libnvidia-ml.so.1
/usr/lib64/libnvidia-ml.so.580.142
/usr/lib64/libnvidia-nvvm.so
/usr/lib64/libnvidia-nvvm.so.4
/usr/lib64/libnvidia-nvvm.so.580.142
/usr/lib64/libnvidia-nvvm70.so.4
/usr/lib64/libnvidia-opencl.so.1
/usr/lib64/libnvidia-opencl.so.580.142
/usr/lib64/libnvidia-opticalflow.so.1
/usr/lib64/libnvidia-opticalflow.so.580.142
/usr/lib64/libnvidia-ptxjitcompiler.so.1
/usr/lib64/libnvidia-ptxjitcompiler.so.580.142
/usr/lib64/libnvidia-sandboxutils.so.1
/usr/lib64/libnvidia-sandboxutils.so.580.142

-- nvidia-persistenced-580.142-1.fc42.x86_64 --
/usr/bin/nvidia-persistenced
/usr/lib/systemd/system/nvidia-persistenced.service
/usr/share/doc/nvidia-persistenced
/usr/share/doc/nvidia-persistenced/README
/usr/share/licenses/nvidia-persistenced
/usr/share/licenses/nvidia-persistenced/COPYING
/usr/share/man/man1/nvidia-persistenced.1.gz

-- xorg-x11-drv-nvidia-libs-580.142-1.fc42.x86_64 --
/usr/lib64/gbm
/usr/lib64/gbm/nvidia-drm_gbm.so
/usr/lib64/libEGL_nvidia.so.0
/usr/lib64/libEGL_nvidia.so.580.142
/usr/lib64/libGLESv1_CM_nvidia.so.1
/usr/lib64/libGLESv1_CM_nvidia.so.580.142
/usr/lib64/libGLESv2_nvidia.so.2
/usr/lib64/libGLESv2_nvidia.so.580.142
/usr/lib64/libGLX_nvidia.so.0
/usr/lib64/libGLX_nvidia.so.580.142
/usr/lib64/libnvidia-allocator.so.1
/usr/lib64/libnvidia-allocator.so.580.142
/usr/lib64/libnvidia-api.so.1
/usr/lib64/libnvidia-cfg.so.1
/usr/lib64/libnvidia-cfg.so.580.142
/usr/lib64/libnvidia-eglcore.so.580.142
/usr/lib64/libnvidia-fbc.so.1
/usr/lib64/libnvidia-fbc.so.580.142
/usr/lib64/libnvidia-glcore.so.580.142
/usr/lib64/libnvidia-glsi.so.580.142
/usr/lib64/libnvidia-glvkspirv.so.580.142
/usr/lib64/libnvidia-gpucomp.so.580.142
/usr/lib64/libnvidia-ngx.so.1
/usr/lib64/libnvidia-ngx.so.580.142
/usr/lib64/libnvidia-pkcs11-openssl3.so.580.142
/usr/lib64/libnvidia-present.so.580.142
/usr/lib64/libnvidia-rtcore.so.580.142
/usr/lib64/libnvidia-tls.so.580.142
/usr/lib64/libnvidia-vksc-core.so.1
/usr/lib64/libnvidia-vksc-core.so.580.142
/usr/lib64/libnvoptix.so.1
/usr/lib64/libnvoptix.so.580.142
/usr/lib64/nvidia/wine
/usr/lib64/nvidia/wine/_nvngx.dll
/usr/lib64/nvidia/wine/nvngx.dll
/usr/lib64/nvidia/wine/nvngx_dlssg.dll
/usr/lib64/vdpau/libvdpau_nvidia.so.1
/usr/lib64/vdpau/libvdpau_nvidia.so.580.142
/usr/share/glvnd/egl_vendor.d/10_nvidia.json
/usr/share/vulkan/icd.d/nvidia_icd.x86_64.json
/usr/share/vulkan/implicit_layer.d/nvidia_layers.json
/usr/share/vulkansc/icd.d/nvidia_icd_vksc.x86_64.json

-- xorg-x11-drv-nvidia-kmodsrc-580.142-1.fc42.x86_64 --
/usr/lib/rpm/macros.d/macros.xorg-x11-drv-nvidia-kmodsrc
/usr/share/nvidia-kmod-580.142
/usr/share/nvidia-kmod-580.142/nvidia-kmod-580.142-x86_64.tar.xz

-- nvidia-settings-580.142-1.fc42.x86_64 --
/etc/xdg/autostart/nvidia-settings-user.desktop
/usr/bin/nvidia-settings
/usr/lib64/libnvidia-gtk3.so.580.142
/usr/lib64/libnvidia-wayland-client.so.580.142
/usr/share/applications/nvidia-settings.desktop
/usr/share/doc/nvidia-settings
/usr/share/doc/nvidia-settings/FRAMELOCK.txt
/usr/share/doc/nvidia-settings/NV-CONTROL-API.txt
/usr/share/man/man1/nvidia-settings.1.gz
/usr/share/metainfo/nvidia-settings.appdata.xml
/usr/share/pixmaps/nvidia-settings.png

-- xorg-x11-drv-nvidia-580.142-1.fc42.x86_64 --
/etc/nvidia
/usr/bin/nvidia-bug-report.sh
/usr/bin/nvidia-pcc
/usr/lib/dracut/dracut.conf.d/99-nvidia-dracut.conf
/usr/lib/firmware
/usr/lib/firmware/nvidia
/usr/lib/firmware/nvidia/580.142
/usr/lib/firmware/nvidia/580.142/gsp_ga10x.bin
/usr/lib/firmware/nvidia/580.142/gsp_tu10x.bin
/usr/lib/nvidia
/usr/lib/nvidia/alternate-install-present
/usr/lib/systemd/system/nvidia-fallback.service
/usr/lib/udev/rules.d/10-nvidia.rules
/usr/lib/udev/rules.d/80-nvidia-pm.rules
/usr/share/doc/xorg-x11-drv-nvidia
/usr/share/doc/xorg-x11-drv-nvidia/NVIDIA_Changelog
/usr/share/doc/xorg-x11-drv-nvidia/README.txt
/usr/share/doc/xorg-x11-drv-nvidia/html
/usr/share/doc/xorg-x11-drv-nvidia/html/acknowledgements.html
/usr/share/doc/xorg-x11-drv-nvidia/html/addressingcapabilities.html
/usr/share/doc/xorg-x11-drv-nvidia/html/addtlresources.html
/usr/share/doc/xorg-x11-drv-nvidia/html/appendices.html
/usr/share/doc/xorg-x11-drv-nvidia/html/audiosupport.html
/usr/share/doc/xorg-x11-drv-nvidia/html/commonproblems.html
/usr/share/doc/xorg-x11-drv-nvidia/html/configlaptop.html
/usr/share/doc/xorg-x11-drv-nvidia/html/configmultxscreens.html
/usr/share/doc/xorg-x11-drv-nvidia/html/configtwinview.html
/usr/share/doc/xorg-x11-drv-nvidia/html/depth30.html
/usr/share/doc/xorg-x11-drv-nvidia/html/displaydevicenames.html
/usr/share/doc/xorg-x11-drv-nvidia/html/dma_issues.html
/usr/share/doc/xorg-x11-drv-nvidia/html/dpi.html
/usr/share/doc/xorg-x11-drv-nvidia/html/dynamicboost.html
/usr/share/doc/xorg-x11-drv-nvidia/html/dynamicpowermanagement.html
/usr/share/doc/xorg-x11-drv-nvidia/html/editxconfig.html
/usr/share/doc/xorg-x11-drv-nvidia/html/egpu.html
/usr/share/doc/xorg-x11-drv-nvidia/html/faq.html
/usr/share/doc/xorg-x11-drv-nvidia/html/flippingubb.html
/usr/share/doc/xorg-x11-drv-nvidia/html/framelock.html
/usr/share/doc/xorg-x11-drv-nvidia/html/gbm.html
/usr/share/doc/xorg-x11-drv-nvidia/html/glxsupport.html
/usr/share/doc/xorg-x11-drv-nvidia/html/gpunames.html
/usr/share/doc/xorg-x11-drv-nvidia/html/gsp.html
/usr/share/doc/xorg-x11-drv-nvidia/html/i2c.html
/usr/share/doc/xorg-x11-drv-nvidia/html/index.html
/usr/share/doc/xorg-x11-drv-nvidia/html/installationandconfiguration.html
/usr/share/doc/xorg-x11-drv-nvidia/html/installdriver.html
/usr/share/doc/xorg-x11-drv-nvidia/html/installedcomponents.html
/usr/share/doc/xorg-x11-drv-nvidia/html/introduction.html
/usr/share/doc/xorg-x11-drv-nvidia/html/kernel_open.html
/usr/share/doc/xorg-x11-drv-nvidia/html/kms.html
/usr/share/doc/xorg-x11-drv-nvidia/html/knownissues.html
/usr/share/doc/xorg-x11-drv-nvidia/html/minimumrequirements.html
/usr/share/doc/xorg-x11-drv-nvidia/html/newusertips.html
/usr/share/doc/xorg-x11-drv-nvidia/html/ngx.html
/usr/share/doc/xorg-x11-drv-nvidia/html/nvidia-debugdump.html
/usr/share/doc/xorg-x11-drv-nvidia/html/nvidia-ml.html
/usr/share/doc/xorg-x11-drv-nvidia/html/nvidia-peermem.html
/usr/share/doc/xorg-x11-drv-nvidia/html/nvidia-persistenced.html
/usr/share/doc/xorg-x11-drv-nvidia/html/nvidia-smi.html
/usr/share/doc/xorg-x11-drv-nvidia/html/nvidiasettings.html
/usr/share/doc/xorg-x11-drv-nvidia/html/nvpresent.html
/usr/share/doc/xorg-x11-drv-nvidia/html/openglenvvariables.html
/usr/share/doc/xorg-x11-drv-nvidia/html/optimus.html
/usr/share/doc/xorg-x11-drv-nvidia/html/powermanagement.html
/usr/share/doc/xorg-x11-drv-nvidia/html/primerenderoffload.html
/usr/share/doc/xorg-x11-drv-nvidia/html/procinterface.html
/usr/share/doc/xorg-x11-drv-nvidia/html/profiles.html
/usr/share/doc/xorg-x11-drv-nvidia/html/programmingmodes.html
/usr/share/doc/xorg-x11-drv-nvidia/html/randr14.html
/usr/share/doc/xorg-x11-drv-nvidia/html/retpoline.html
/usr/share/doc/xorg-x11-drv-nvidia/html/selectdriver.html
/usr/share/doc/xorg-x11-drv-nvidia/html/sli.html
/usr/share/doc/xorg-x11-drv-nvidia/html/supportedchips.html
/usr/share/doc/xorg-x11-drv-nvidia/html/vdpausupport.html
/usr/share/doc/xorg-x11-drv-nvidia/html/wayland-issues.html
/usr/share/doc/xorg-x11-drv-nvidia/html/xcompositeextension.html
/usr/share/doc/xorg-x11-drv-nvidia/html/xconfigoptions.html
/usr/share/doc/xorg-x11-drv-nvidia/html/xineramaglx.html
/usr/share/doc/xorg-x11-drv-nvidia/html/xrandrextension.html
/usr/share/doc/xorg-x11-drv-nvidia/html/xwayland.html
/usr/share/doc/xorg-x11-drv-nvidia/nvidia-application-profiles-580.142-rc
/usr/share/licenses/xorg-x11-drv-nvidia
/usr/share/licenses/xorg-x11-drv-nvidia/LICENSE
/usr/share/metainfo/xorg-x11-drv-nvidia.metainfo.xml
/usr/share/nvidia
/usr/share/nvidia/nvidia-application-profiles-580.142-key-documentation
/usr/share/nvidia/nvidia-application-profiles-580.142-rc
/usr/share/nvidia/nvidia-application-profiles-key-documentation
/usr/share/nvidia/nvidia-application-profiles-rc
/usr/share/nvidia/nvoptix.bin
/usr/share/pixmaps/xorg-x11-drv-nvidia.png

-- akmod-nvidia-580.142-1.fc42.x86_64 --
/usr/src/akmods/nvidia-kmod-580.142-1.fc42.src.rpm
/usr/src/akmods/nvidia-kmod.latest

-- xorg-x11-drv-nvidia-cuda-580.142-1.fc42.x86_64 --
/usr/bin/nvidia-cuda-mps-control
/usr/bin/nvidia-cuda-mps-server
/usr/bin/nvidia-debugdump
/usr/bin/nvidia-ngx-updater
/usr/bin/nvidia-smi
/usr/share/licenses/xorg-x11-drv-nvidia-cuda
/usr/share/licenses/xorg-x11-drv-nvidia-cuda/LICENSE
/usr/share/man/man1/nvidia-cuda-mps-control.1.gz
/usr/share/man/man1/nvidia-smi.1.gz
/usr/share/nvidia/files.d
/usr/share/nvidia/files.d/sandboxutils-filelist.json

-- xorg-x11-drv-nvidia-power-580.142-1.fc42.x86_64 --
/usr/bin/nvidia-powerd
/usr/bin/nvidia-sleep.sh
/usr/lib/modprobe.d/nvidia-power-management.conf
/usr/lib/systemd/system-preset/70-nvidia.preset
/usr/lib/systemd/system-sleep/nvidia
/usr/lib/systemd/system/nvidia-hibernate.service
/usr/lib/systemd/system/nvidia-powerd.service
/usr/lib/systemd/system/nvidia-resume.service
/usr/lib/systemd/system/nvidia-suspend-then-hibernate.service
/usr/lib/systemd/system/nvidia-suspend.service
/usr/share/dbus-1/system.d/nvidia-dbus.conf

-- kmod-nvidia-6.19.8-100.fc42.x86_64-580.142-1.fc42.x86_64 --
/lib/modules/6.19.8-100.fc42.x86_64/extra
/lib/modules/6.19.8-100.fc42.x86_64/extra/nvidia
/lib/modules/6.19.8-100.fc42.x86_64/extra/nvidia/nvidia-drm.ko.xz
/lib/modules/6.19.8-100.fc42.x86_64/extra/nvidia/nvidia-modeset.ko.xz
/lib/modules/6.19.8-100.fc42.x86_64/extra/nvidia/nvidia-peermem.ko.xz
/lib/modules/6.19.8-100.fc42.x86_64/extra/nvidia/nvidia-uvm.ko.xz
/lib/modules/6.19.8-100.fc42.x86_64/extra/nvidia/nvidia.ko.xz

-- kmod-nvidia-6.19.10-100.fc42.x86_64-580.142-1.fc42.x86_64 --
/lib/modules/6.19.10-100.fc42.x86_64/extra
/lib/modules/6.19.10-100.fc42.x86_64/extra/nvidia
/lib/modules/6.19.10-100.fc42.x86_64/extra/nvidia/nvidia-drm.ko.xz
/lib/modules/6.19.10-100.fc42.x86_64/extra/nvidia/nvidia-modeset.ko.xz
/lib/modules/6.19.10-100.fc42.x86_64/extra/nvidia/nvidia-peermem.ko.xz
/lib/modules/6.19.10-100.fc42.x86_64/extra/nvidia/nvidia-uvm.ko.xz
/lib/modules/6.19.10-100.fc42.x86_64/extra/nvidia/nvidia.ko.xz
```

## 忽然意识到到，每一次内核升级，nvidia-rpm 都会自动配置上
```txt
🧀  sudo rpm -ql kmod-nvidia-6.19.13-100.fc42.x86_64-580.142-2.fc42.x86_64

/lib/modules/6.19.13-100.fc42.x86_64/extra
/lib/modules/6.19.13-100.fc42.x86_64/extra/nvidia
/lib/modules/6.19.13-100.fc42.x86_64/extra/nvidia/nvidia-drm.ko.xz
/lib/modules/6.19.13-100.fc42.x86_64/extra/nvidia/nvidia-modeset.ko.xz
/lib/modules/6.19.13-100.fc42.x86_64/extra/nvidia/nvidia-peermem.ko.xz
/lib/modules/6.19.13-100.fc42.x86_64/extra/nvidia/nvidia-uvm.ko.xz
/lib/modules/6.19.13-100.fc42.x86_64/extra/nvidia/nvidia.ko.xz
```

## 切换驱动从闭源驱动到开源驱动

### 包状态检查

检查 NVIDIA 相关包:

```sh
dnf list --installed "*nvidia*" "*akmod*" "*kmod*"
echo a | sudo -S rpm -qa | rg -i "nvidia|akmod|kmod|cuda|dkms|kernel-devel|kernel-headers"
```

发现旧包包括:

```text
akmod-nvidia-3:580.142-2.fc43.x86_64
kmod-nvidia-6.19.10-100.fc42.x86_64-3:580.142-2.fc42.x86_64
kmod-nvidia-6.19.13-100.fc42.x86_64-3:580.142-2.fc42.x86_64
kmod-nvidia-6.19.13-200.fc43.x86_64-3:580.142-2.fc43.x86_64
xorg-x11-drv-nvidia-kmodsrc-3:580.142-1.fc43.x86_64
```

查询 open module 包:

```sh
dnf search nvidia-open
dnf info nvidia-open kmod-nvidia-open-dkms
dnf install --assumeno nvidia-open
dnf install --assumeno --disablerepo=cuda-fedora41-x86_64 kmod-nvidia-open-dkms-3:580.159.03-1.fc42
```

`nvidia-open` 元包会和 RPM Fusion 的 `xorg-x11-drv-nvidia-cuda` 冲突，所以没有使用该元包。
选择只安装 `kmod-nvidia-open-dkms`，同时让现有 RPM Fusion 用户态包升级到 `580.159.03`。

### 已执行的修改

安装 open DKMS 模块:

```sh
echo a | sudo -S dnf install -y --disablerepo=cuda-fedora41-x86_64 kmod-nvidia-open-dkms-3:580.159.03-1.fc42
```

该事务做了以下事情:

- 安装 `dkms-3.4.0-1.fc43`
- 安装 `kmod-nvidia-open-dkms-580.159.03`
- 将 RPM Fusion NVIDIA 用户态包从 `580.142` 升级到 `580.159.03`
- DKMS 为当前内核构建 `nvidia/580.159.03`
- 执行了 `dracut --regenerate-all --force`

移除旧闭源模块包:

```sh
echo a | sudo -S dnf remove -y \
  akmod-nvidia \
  kmod-nvidia-6.19.10-100.fc42.x86_64 \
  kmod-nvidia-6.19.13-100.fc42.x86_64 \
  kmod-nvidia-6.19.13-200.fc43.x86_64 \
  xorg-x11-drv-nvidia-kmodsrc
```

刷新模块依赖和 initramfs:

```sh
echo a | sudo -S depmod -a
echo a | sudo -S dracut --regenerate-all --force
```

### 当前包状态

当前 NVIDIA 相关包:

```text
dkms-0:3.4.0-1.fc43.noarch
kmod-nvidia-open-dkms-3:580.159.03-1.fc42.noarch
nvidia-modprobe-3:580.159.03-1.fc43.x86_64
nvidia-persistenced-3:580.159.03-1.fc43.x86_64
nvidia-settings-3:580.159.03-1.fc43.x86_64
xorg-x11-drv-nvidia-3:580.159.03-1.fc43.x86_64
xorg-x11-drv-nvidia-cuda-3:580.159.03-1.fc43.x86_64
xorg-x11-drv-nvidia-cuda-libs-3:580.159.03-1.fc43.x86_64
xorg-x11-drv-nvidia-libs-3:580.159.03-1.fc43.x86_64
xorg-x11-drv-nvidia-power-3:580.159.03-1.fc43.x86_64
```

当前磁盘上的模块:

```sh
modinfo nvidia | rg -i "filename|version|license|srcversion"
```

结果:

```text
filename:       /lib/modules/6.19.13-200.fc43.x86_64/extra/nvidia.ko.xz
version:        580.159.03
license:        Dual MIT/GPL
srcversion:     23DC68F0209390A3D14E0FA
```

这说明磁盘上的 `nvidia.ko` 已经切换为 open kernel module。

### 当前剩余状态

运行中的内核模块仍是旧版本:

```sh
cat /sys/module/nvidia/version /sys/module/nvidia/taint 2>/dev/null
```

当前输出:

```text
580.142
POE
```

因此当前 `nvidia-smi` 报:

```text
Failed to initialize NVML: Driver/library version mismatch
NVML library version: 580.159
```

含义: 用户态库已经升级到 `580.159`，但运行中的内核模块仍是旧的 `580.142`。需要重新加载模块或重启后才能生效。
