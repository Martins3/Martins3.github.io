
pip3 install pyelftools
```sh
meson build
meson setup --reconfigure build --prefix $PWD/install -Dexamples=all -Dplatform=generic  # 如果是 mac
ninja -C build
echo  64 | sudo tee /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages
```


```txt
apps:
        dumpcap, graph, pdump, proc-info, test-acl, test-bbdev, test-cmdline, test-compress-perf,
        test-crypto-perf, test-dma-perf, test-eventdev, test-fib, test-flow-perf, test-gpudev, test-mldev, test-pipeline,
        test-pmd, test-regex, test-sad, test-security-perf, test,

Message:
=================
Libraries Enabled
=================

libs:
        log, kvargs, argparse, telemetry, eal, ptr_compress, ring, rcu,
        mempool, mbuf, net, meter, ethdev, pci, cmdline, metrics,
        hash, timer, acl, bbdev, bitratestats, bpf, cfgfile, compressdev,
        cryptodev, distributor, dmadev, efd, eventdev, dispatcher, gpudev, gro,
        gso, ip_frag, jobstats, latencystats, lpm, member, pcapng, power,
        rawdev, regexdev, mldev, rib, reorder, sched, security, stack,
        vhost, ipsec, pdcp, fib, port, pdump, table, pipeline,
        graph, node,

Message:
===============
Drivers Enabled
===============

common:
        cpt, dpaax, iavf, idpf, ionic, octeontx, cnxk, nfp,
        nitrox, qat, sfc_efx,
bus:
        auxiliary, cdx, dpaa, fslmc, ifpga, pci, platform, uacce,
        vdev, vmbus,
mempool:
        bucket, cnxk, dpaa, dpaa2, octeontx, ring, stack,
dma:
        cnxk, dpaa, dpaa2, hisilicon, odm, skeleton,
net:
        af_packet, ark, atlantic, avp, axgbe, bnx2x, bnxt, bond,
        cnxk, cpfl, cxgbe, dpaa, dpaa2, e1000, ena, enetc,
        enetfec, enic, failsafe, fm10k, gve, hinic, hns3, i40e,
        iavf, ice, idpf, igc, ionic, ixgbe, memif, netvsc,
        nfp, ngbe, null, octeontx, octeon_ep, pcap, pfe, qede,
        ring, sfc, softnic, tap, thunderx, txgbe, vdev_netvsc, vhost,
        virtio, vmxnet3,
raw:
        cnxk_bphy, cnxk_gpio, dpaa2_cmdif, ntb, skeleton,
crypto:
        bcmfs, caam_jr, cnxk, dpaa_sec, dpaa2_sec, ionic, nitrox, null,
        octeontx, scheduler, virtio,
compress:
        nitrox, octeontx, zlib,
regex:
        cn9k,
ml:
        cnxk,
vdpa:
        ifc, nfp, sfc,
event:
        cnxk, dpaa, dpaa2, dsw, opdl, skeleton, sw, octeontx,

baseband:
        acc, fpga_5gnr_fec, fpga_lte_fec, la12xx, null, turbo_sw,
gpu:


Message:
=================
Content Skipped
=================

apps:

libs:

drivers:
        common/mvep:    missing dependency, "libmusdk"
        common/mlx5:    missing dependency, "mlx5"
        crypto/qat:     missing dependency for Arm, libcrypto
        dma/idxd:       only supported on x86
        dma/ioat:       only supported on x86
        net/af_xdp:     missing dependency, "libxdp >=1.2.2" and "libbpf"
        net/ipn3ke:     missing dependency, "libfdt"
        net/mana:       only supported on x86 Linux
        net/mlx4:       missing dependency, "mlx4"
        net/mlx5:       missing internal dependency, "common_mlx5"
        net/mvneta:     missing dependency, "libmusdk"
        net/mvpp2:      missing dependency, "libmusdk"
        net/nfb:        missing dependency, "libnfb"
        net/ntnic:      only supported on x86_64 Linux
        raw/ifpga:      missing dependency, "libfdt"
        crypto/armv8:   missing dependency, "libAArch64crypto"
        crypto/ccp:     missing dependency, "libcrypto"
        crypto/ipsec_mb:        missing dependency, "libIPSec_MB"
        crypto/mlx5:    missing internal dependency, "common_mlx5"
        crypto/mvsam:   missing dependency, "libmusdk"
        crypto/openssl: missing dependency, "libcrypto"
        crypto/uadk:    missing dependency, "libwd"
        compress/isal:  missing dependency, "libisal"
        compress/mlx5:  missing internal dependency, "common_mlx5"
        compress/uadk:  missing dependency, "libwd"
        regex/mlx5:     missing internal dependency, "common_mlx5"
        vdpa/mlx5:      missing internal dependency, "common_mlx5"
        event/dlb2:     only supported on x86_64 Linux
        gpu/cuda:       missing dependency, "cuda.h"

```

直接给干回来了
./usertools/dpdk-devbind.py --bind=virtio-pci 00:04.0
这个才是应该高的东西
sudo ./usertools/dpdk-devbind.py --bind=vfio-pci 06:00.0

./usertools/dpdk-devbind.py --bind=igb_uio eth0

添加我们的 install 的位置:

PKGCONF := pkg-config --define-prefix --with-path=/root/dpdk/install/lib64/pkgconfig

可以编译，但是无法运行的问题:
echo /root/dpdk/install/lib64 >  /etc/ld.so.conf.d/dpdk.conf

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/root/dpdk/install/lib64

其实其中的 examples 已经够多了。

## /root/dpdk/build/examples/dpdk-devbind.py
可以通过 /root/dpdk/build/examples/dpdk-devbind.py 来检查 device 状态

```txt
➜  examples git:(main) ✗ dpdk-devbind.py -s
lspci: Unable to load libkmod resources: error -2
lspci: Unable to load libkmod resources: error -2
lspci: Unable to load libkmod resources: error -2
lspci: Unable to load libkmod resources: error -2
lspci: Unable to load libkmod resources: error -2
lspci: Unable to load libkmod resources: error -2
lspci: Unable to load libkmod resources: error -2
lspci: Unable to load libkmod resources: error -2
lspci: Unable to load libkmod resources: error -2
lspci: Unable to load libkmod resources: error -2

Network devices using kernel driver
===================================
0000:00:02.0 'Virtio network device 1000' if=enp0s2 drv=virtio-pci unused=vfio-pci *Active*
0000:00:03.0 'Virtio network device 1000' if=enp0s3 drv=virtio-pci unused=vfio-pci *Active*
0000:00:04.0 'Virtio network device 1000' if=enp0s4 drv=virtio-pci unused=vfio-pci

No 'Baseband' devices detected
==============================

No 'Crypto' devices detected
============================

No 'DMA' devices detected
=========================

No 'Eventdev' devices detected
==============================

No 'Mempool' devices detected
=============================

No 'Compress' devices detected
==============================

Misc (rawdev) devices using kernel driver
=========================================
0000:00:01.0 'Virtio block device 1001' drv=virtio-pci unused=vfio-pci
0000:00:0b.0 'Virtio block device 1001' drv=virtio-pci unused=vfio-pci
0000:00:0c.0 'Virtio block device 1001' drv=virtio-pci unused=vfio-pci

No 'Regex' devices detected
===========================

No 'ML' devices detected
```
但是为什么总是有

```txt
lspci: Unable to load libkmod resources: error -2
```

## 还是在虚拟机中，把这些基本的 examples 都搞完吧

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
