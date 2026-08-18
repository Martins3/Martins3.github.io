## fedora 42 中安装
```txt
1. 启用 RPM Fusion 仓库：
sudo dnf install -y https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-42.noarch.rpm https://download1.rpmfusion.org/nonfree/fedo
ra/rpmfusion-nonfree-release-42.noarch.rpm
2. 安装 NVIDIA 驱动包：
sudo dnf install -y akmod-nvidia xorg-x11-drv-nvidia xorg-x11-drv-nvidia-cuda
3. 构建内核模块：
sudo akmods --force
4. 黑名单 nouveau 驱动：
sudo sh -c 'cat > /etc/modprobe.d/blacklist-nouveau.conf << EOF
blacklist nouveau
options nouveau modeset=0
EOF'
5. 加载 NVIDIA 内核模块：
sudo modprobe nvidia nvidia-uvm nvidia-modeset nvidia-drm
```

整个做的都很顺利，但是发现物理机中存在这些报错:
```txt
[  428.620346] nvidia 0000:01:00.0: vgaarb: VGA decodes changed: olddecodes=none,decodes=none:owns=none
[  428.823493] NVRM: loading NVIDIA UNIX x86_64 Kernel Module  580.142  Tue Mar  3 20:04:04 UTC 2026
[  428.868668] nvidia_uvm: module uses symbols nvUvmInterfaceDisableAccessCntr from proprietary module nvidia, inheriting taint.
[  442.330779] ACPI BIOS Error (bug): Failure creating named object [\_SB.PC00.PEG1.PEGP._DSM.USRG], AE_ALREADY_EXISTS (20250807/dsfield-184)
[  442.330792] ACPI Error: AE_ALREADY_EXISTS, CreateBufferField failure (20250807/dswload2-477)
[  442.330797] ACPI Error: Aborting method \_SB.PC00.PEG1.PEGP._DSM due to previous error (AE_ALREADY_EXISTS) (20250807/psparse-529)
[  455.224848] ACPI BIOS Error (bug): Failure creating named object [\_SB.PC00.PEG1.PEGP._DSM.USRG], AE_ALREADY_EXISTS (20250807/dsfield-184)
[  455.224865] ACPI Error: AE_ALREADY_EXISTS, CreateBufferField failure (20250807/dswload2-477)
[  455.224870] ACPI Error: Aborting method \_SB.PC00.PEG1.PEGP._DSM due to previous error (AE_ALREADY_EXISTS) (20250807/psparse-529)
[  493.888165] ACPI BIOS Error (bug): Failure creating named object [\_SB.PC00.PEG1.PEGP._DSM.USRG], AE_ALREADY_EXISTS (20250807/dsfield-184)
[  493.888182] ACPI Error: AE_ALREADY_EXISTS, CreateBufferField failure (20250807/dswload2-477)
[  493.888187] ACPI Error: Aborting method \_SB.PC00.PEG1.PEGP._DSM due to previous error (AE_ALREADY_EXISTS) (20250807/psparse-529)
```

