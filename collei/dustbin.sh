#!/usr/bin/env bash
set -E -e -u -o pipefail

function setup_numa_node() {
	#  -numa node,cpus=0-3,mem=4G: Parameter -numa node,mem is not supported by this machine type
	#  曾经很 nb 的配置，结果发现 arm 上不可以用
	#  而且进一步发现，这不是 qemu 推荐的模式
	local node0_cpus
	local node1_cpus
	node0_cpus=$(printf "cpus=%s," {1..127..2})
	node1_cpus=$(printf "cpus=%s," {0..127..2})
	# 将结尾的 , 去掉
	node0_cpus=${node0_cpus%%,}
	node1_cpus=${node1_cpus%%,}
	arg_mem_cpu+=" -numa node,$node0_cpus,memdev=mem0,nodeid=0"
	arg_mem_cpu+=" -numa node,$node1_cpus,memdev=mem1,nodeid=1 "
}

# 所以，这个东西到底怎么配置的 !
# 正好用这个来测试操作系统的 scheduler 合并了
arg_mem_cpu="-m 8G -smp 8,sockets=2,cores=2,threads=2,maxcpus=8"

# 第一个测试
# 通过 reserve = false 让 mmap 携带参数 MAP_NORESERVE，从而可以模拟超级大内存的 Guest
# arg_mem_cpu=" -m 32G -smp cpus=32"
# arg_mem_cpu+=" -object memory-backend-ram,size=16G,id=m0,prealloc=false -numa node,memdev=m0,cpus=0-15,nodeid=0"
# arg_mem_cpu+=" -object memory-backend-ram,size=16G,id=m1,reserve=false -numa node,memdev=m1,cpus=16-31,nodeid=1"

# 第二个测试
# arg_mem_cpu+=" -object memory-backend-ram,size=4G,id=m2,reserve=false -numa node,memdev=m2,cpus=4,nodeid=2"
# arg_mem_cpu+=" -numa node,cpus=5,nodeid=3" # 只有 CPU ，但是没有内存

# 第三个测试
arg_mem_cpu="-smp 64,maxcpus=240,sockets=240,dies=1,clusters=1,cores=1,threads=1 "
arg_mem_cpu+="-m size=67108864k,slots=255,maxmem=4194304000k "
arg_mem_cpu+='-object memory-backend-ram,id=ram-node0,prealloc=false,share=true,size=8589934592 '
arg_mem_cpu+='-object memory-backend-ram,id=ram-node1,prealloc=false,share=true,size=8589934592 '
arg_mem_cpu+='-object memory-backend-ram,id=ram-node2,prealloc=false,share=true,size=8589934592 '
arg_mem_cpu+='-object memory-backend-ram,id=ram-node3,prealloc=false,share=true,size=8589934592 '
arg_mem_cpu+='-object memory-backend-ram,id=ram-node4,prealloc=false,share=true,size=8589934592 '
arg_mem_cpu+='-object memory-backend-ram,id=ram-node5,prealloc=false,share=true,size=8589934592 '
arg_mem_cpu+='-object memory-backend-ram,id=ram-node6,prealloc=false,share=true,size=8589934592 '
arg_mem_cpu+='-object memory-backend-ram,id=ram-node7,prealloc=false,share=true,size=8589934592 '
arg_mem_cpu+="-numa node,nodeid=0,cpus=0-7,memdev=ram-node0 "
arg_mem_cpu+="-numa node,nodeid=1,cpus=8-15,memdev=ram-node1 "
arg_mem_cpu+="-numa node,nodeid=2,cpus=16-23,memdev=ram-node2 "
arg_mem_cpu+="-numa node,nodeid=3,cpus=24-31,memdev=ram-node3 "
arg_mem_cpu+="-numa node,nodeid=4,cpus=32-39,memdev=ram-node4 "
arg_mem_cpu+="-numa node,nodeid=5,cpus=40-47,memdev=ram-node5 "
arg_mem_cpu+="-numa node,nodeid=6,cpus=48-55,memdev=ram-node6 "
arg_mem_cpu+="-numa node,nodeid=7,cpus=56-239,memdev=ram-node7 "
