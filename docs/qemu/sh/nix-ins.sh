#!/bin/bash

parted /dev/sda -- mklabel msdos
parted /dev/sda -- mkpart primary 1MiB -8GiB
mkfs.ext4 -L nixos /dev/sda1
mount /dev/disk/by-label/nixos /mnt
nixos-generate-config --root /mnt

export http_proxy=http://10.90.50.75:8889 && export https_proxy=http://10.90.50.75:8889
