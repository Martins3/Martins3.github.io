#!/usr/bin/env bash
set -E -e -u -o pipefail
PROGDIR=$(readlink -m "$(dirname "$0")")
cd "$PROGDIR"

docker build --network=host --tag fedora:martins3 - <Dockerfile


# TODO 不知道发生了什么，如果采用自己创建的 network ，iperf3 性能特别差
# podman network create -d bridge hh
podman run  --name genshin --net=hh --privileged -it --rm --workdir /root -v "$(pwd)":/root fedora:martins3
# docker run --name genshin --privileged -it --rm --workdir /root -v "$(pwd)":/root fedora:martins3

# 有趣，测试了下，甚至是可以控制网卡的名称的，自动加入到一个网桥上，
# 可以连接上网络中
# sudo ovs−docker add−port br-in eth1 genshin1 −−ipaddress=10.0.1.1/16
# sudo ovs−docker add−port br-in eth1 genshin2 −−ipaddress=10.0.1.2/16
