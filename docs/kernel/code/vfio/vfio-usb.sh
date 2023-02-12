#!/usr/bin/env bash
shopt -s nullglob
for usb_ctrl in /sys/bus/pci/devices/*/usb*; do
  pci_path=${usb_ctrl%/*}
  iommu_group=$(readlink $pci_path/iommu_group)
  echo "Bus $(cat $usb_ctrl/busnum) --> ${pci_path##*/} (IOMMU group ${iommu_group##*/})"
  lsusb -s ${usb_ctrl#*/usb}:
  echo
done
