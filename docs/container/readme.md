## 问题
设置 rootless 这么复杂吗?
https://github.com/containers/podman/blob/main/docs/tutorials/rootless_tutorial.md

为什么在这里也看到了 slirp4netns
https://github.com/rootless-containers/slirp4netns?tab=readme-ov-file


https://github.com/containers/podman/blob/main/docs/tutorials/socket_activation.md

好的，也就是这几个了:
https://github.com/containers/podman/blob/main/docs/tutorials/basic_networking.md

```txt
create the bundle
$ mkdir -p /mycontainer/rootfs
# [ab]use Docker to copy a root fs into the bundle
$ docker export $(docker create busybox) | tar -C /mycontainer/rootfs -xvf -
# create the specification, by default sh will be the entrypoint of the container
$ cd /mycontainer
$ runc spec
# launch the container
$ sudo -i
$ cd /mycontainer
$ runc run mycontainerid
# list containers
$ runc list
# stop the container
$ runc kill mycontainerid
# cleanup
$ runc delete mycontainerid
```

## lxc lxd vs runc youki
https://lwn.net/Articles/907613/
https://github.com/lxc/lxc


## 看看 distrobox 这个项目
https://github.com/89luca89/distrobox

## 一个小项目
https://github.com/edera-dev/styrolite/blob/main/src/capability.rs

## Apple 上的
https://news.ycombinator.com/item?id=44229239


## 有的容器还有 systemd
```txt
podman pull quay.io/centos-bootc/centos-bootc:stream9
podman run -it centos-bootc:stream9
```

## 这个是做什么的，codex 会用这个东西
https://github.com/containers/bubblewrap

## harbor 总是搭建失败，我想在 ai 时代，这个不是一个什么大问题吧

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
