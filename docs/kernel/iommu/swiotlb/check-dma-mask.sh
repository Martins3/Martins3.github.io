#!/usr/bin/env bash
set -E -e -u -o pipefail

# 检查到底那些设备的 dma_mask_bits 是 32 bit 的
for dev in /sys/bus/pci/devices/*; do
	dev_name=$(basename "$dev")
	driver=$(basename "$(readlink "$dev/driver")" 2>/dev/null || echo "no driver")

	if [[ $driver == pcieport || -z $driver ]]; then
		continue
	fi

	if [ -f "$dev/dma_mask_bits" ]; then
		bits=$(cat "$dev/dma_mask_bits")
		if [ "$bits" -eq "32" ]; then
			printf "[x] "
		fi
	fi
	echo "Device: $dev_name (Driver: $driver) - DMA mask: $bits bits"
done

# 联想:
# [x] Device: 0000:00:14.0 (Driver: piix4_smbus) - DMA mask: 32 bits
# [x] Device: 0000:00:18.3 (Driver: k10temp) - DMA mask: 32 bits
# Device: 0000:01:00.0 (Driver: nvidia) - DMA mask: 47 bits
# Device: 0000:01:00.1 (Driver: snd_hda_intel) - DMA mask: 40 bits
# Device: 0000:02:00.0 (Driver: nvme) - DMA mask: 64 bits
# Device: 0000:03:00.0 (Driver: nvme) - DMA mask: 64 bits
# [x] Device: 0000:04:00.0 (Driver: mt7921e) - DMA mask: 32 bits
# Device: 0000:07:00.0 (Driver: r8169) - DMA mask: 64 bits
# Device: 0000:08:00.2 (Driver: ccp) - DMA mask: 48 bits
# Device: 0000:08:00.3 (Driver: xhci_hcd) - DMA mask: 64 bits
# Device: 0000:08:00.4 (Driver: xhci_hcd) - DMA mask: 64 bits
# [x] Device: 0000:08:00.5 (Driver: snd_rpl_pci_acp6x) - DMA mask: 32 bits
# Device: 0000:08:00.6 (Driver: snd_hda_intel) - DMA mask: 40 bits
# Device: 0000:09:00.0 (Driver: xhci_hcd) - DMA mask: 64 bits
#
#
# 13900k:
# Device: 0000:00:02.0 (Driver: i915) - DMA mask: 39 bits
# [x] Device: 0000:00:0a.0 (Driver: intel_vsec) - DMA mask: 32 bits
# Device: 0000:00:14.0 (Driver: xhci_hcd) - DMA mask: 64 bits
# Device: 0000:00:14.3 (Driver: iwlwifi) - DMA mask: 64 bits
# [x] Device: 0000:00:15.0 (Driver: intel-lpss) - DMA mask: 32 bits
# [x] Device: 0000:00:15.1 (Driver: intel-lpss) - DMA mask: 32 bits
# [x] Device: 0000:00:15.2 (Driver: intel-lpss) - DMA mask: 32 bits
# Device: 0000:00:16.0 (Driver: mei_me) - DMA mask: 64 bits
# Device: 0000:00:17.0 (Driver: ahci) - DMA mask: 64 bits
# Device: 0000:00:1f.3 (Driver: snd_hda_intel) - DMA mask: 64 bits
# [x] Device: 0000:00:1f.4 (Driver: i801_smbus) - DMA mask: 32 bits
# [x] Device: 0000:00:1f.5 (Driver: intel-spi) - DMA mask: 32 bits
# [x] Device: 0000:01:00.0 (Driver: vfio-pci) - DMA mask: 32 bits
# [x] Device: 0000:01:00.1 (Driver: vfio-pci) - DMA mask: 32 bits
# Device: 0000:02:00.0 (Driver: vfio-pci) - DMA mask: 64 bits
# Device: 0000:03:00.0 (Driver: nvme) - DMA mask: 64 bits
# Device: 0000:05:00.0 (Driver: r8169) - DMA mask: 64 bits
# Device: 0000:06:00.0 (Driver: igc) - DMA mask: 64 bits
# Device: 0000:07:00.0 (Driver: nvme) - DMA mask: 64 bits


# 特殊控制的虚拟机:
# [x] Device: 0000:00:01.2 (Driver: uhci_hcd) - DMA mask: 32 bits
# Device: 0000:00:02.0 (Driver: virtio-pci) - DMA mask: 64 bits
# Device: 0000:00:03.0 (Driver: virtio-pci) - DMA mask: 64 bits
# Device: 0000:00:04.0 (Driver: virtio-pci) - DMA mask: 64 bits
# Device: 0000:00:05.0 (Driver: virtio-pci) - DMA mask: 64 bits
# Device: 0000:00:06.0 (Driver: virtio-pci) - DMA mask: 64 bits
# Device: 0000:00:07.0 (Driver: virtio-pci) - DMA mask: 64 bits
# Device: 0000:00:08.0 (Driver: virtio-pci) - DMA mask: 64 bits
# Device: 0000:00:09.0 (Driver: virtio-pci) - DMA mask: 64 bits
# Device: 0000:00:0b.0 (Driver: virtio-pci) - DMA mask: 64 bits
# Device: 0000:00:0c.0 (Driver: virtio-pci) - DMA mask: 64 bits
# Device: 0000:00:0e.0 (Driver: virtio-pci) - DMA mask: 64 bits
# Device: 0000:00:0f.0 (Driver: virtio-pci) - DMA mask: 64 bits
# Device: 0000:00:10.0 (Driver: xhci_hcd) - DMA mask: 64 bits

