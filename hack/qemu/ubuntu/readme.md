# TODO

add a example how to debug with qemu, a script and ....


## communicate with the guest

- [ ] TODO verify this one by one

- Through `qemu-nbd` to write/read guest's file
- Through libguestfs to mount/unmount qcow2 image
  - `sudo apt-get install libguestfs-tools`
  - `sudo yum install libguestfs-tools`
    - not wotk on Loongson because of the lack of supermin package
    - works fine on Ubuntu 16.04 AMD64
- Through interrnet
  - On default, the guest is connected to the interrnet and is able to `git clone` ...
  - But the guest is inside another Internal network and can not be reached by the host directly, such as
    - Guest IP : 10.0.2.15
    - Host IP: 192.168.1.103
- Through `-net` to make the guest connect to the host
  - may need the `tun` kernel module
    - `sudo modprobe tun`

- With 9p and virtio:
  - https://askubuntu.com/questions/290668/how-to-share-a-folder-between-kvm-host-and-guest-using-virt-manager/1274315#1274315

