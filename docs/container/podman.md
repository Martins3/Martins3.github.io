# Podman 使用指南

https://dockerlabs.collabnix.com/docker/cheatsheet/

```sh
podman run --name genshin -dt -p 8080:80/tcp docker.io/nginx
podman ps
podman inspect
podman logs genshin # 将所有的命令都输出一次
podman top genshin

# 这几个命令必须使用 sudo ，但是会报告 genshin 无法找不到
sudo podman container checkpoint genshin
sudo podman container restore genshin
sudo podman container checkpoint genshin -e /tmp/checkpoint.tar.gz

# 如果将 docker 部署在服务器上，使用此命令登入
# alias x="ssh -t root@12.34.56.78 'docker exec -it wawa zsh'"

# 相同的输入输出
docker attach container_name
```

## podman 基本配置

各种环境的配置都是需要使用，这就是不用依赖 docker daemon 好处吗?

一般只是需要做这个配置就可以了:
```sh
mkdir -p  ~/.config/containers/

cat << _EOF_ > ~/.config/containers/policy.json
{
    "default": [
        {
            "type": "insecureAcceptAnything"
        }
    ]
}
_EOF_
```
## 记录一个奇怪的东西

以前在 nix 中发现无法使用 podman ，后来不知道干了什么，这些问题都自动的消失了。
莫名其妙啊。

```txt
🧀  podman pull quay.io/centos-bootc/centos-bootc:stream9
Error: database graph driver "" does not match our graph driver "overlay": database configuration mismatch
```

```txt
🧀  systemctl status user@1000.service
× user@1000.service - User Manager for UID 1000
     Loaded: loaded (/usr/lib/systemd/system/user@.service; static)
    Drop-In: /usr/lib/systemd/system/user@.service.d
             └─10-login-barrier.conf
     Active: failed (Result: exit-code) since Sun 2024-12-15 11:54:36 CST; 10s ago
       Docs: man:user@.service(5)
    Process: 2526 ExecStart=/usr/lib/systemd/systemd --user (code=exited, status=1/FAILURE)
   Main PID: 2526 (code=exited, status=1/FAILURE)
      Error: 49 (Protocol driver not attached)
        CPU: 5ms

Dec 15 11:54:36 bogon systemd[1]: Starting User Manager for UID 1000...
Dec 15 11:54:36 bogon (systemd)[2526]: pam_unix(systemd-user:session): session opened for user martins3(uid=1000) by (uid=0)
Dec 15 11:54:36 bogon systemd[2526]: Trying to run as user instance, but $XDG_RUNTIME_DIR is not set.
Dec 15 11:54:36 bogon systemd[1]: user@1000.service: Main process exited, code=exited, status=1/FAILURE
Dec 15 11:54:36 bogon systemd[1]: user@1000.service: Failed with result 'exit-code'.
Dec 15 11:54:36 bogon systemd[1]: Failed to start User Manager for UID 1000.
```

podman --cgroup-manager=cgroupfs run hello-world

看看 https://docs.podman.io/en/stable/markdown/podman.1.html

The CGroup manager to use for container cgroups. Supported values are cgroupfs or systemd. Default is systemd unless overridden in the containers.conf file.

https://wiki.archlinux.org/title/Systemd/User

## 发现一些新问题

```txt
🧀  podman run hello-world
Error: short-name "hello-world" did not resolve to an alias and no containers-registries.conf(5) was found
```

## devbox.md 中的 podman 内容也都整理过来吧

## 关键问题

docker 中的 --privilege 到底意味着什么?

发现 docker run  --privilege ，然后在其中创建的问题，
在 docker 外， 只能用 sudo 删掉

发现即便是 podman ，如果不去添加 --privilege ，也会出现分配 userfaultfd 失败
```txt
test-hardlimit: failed to alloc userfaultfd, err: 1 Operation not permitted
: Operation not permitted

🧀  cat /proc/sys/vm/unprivileged_userfaultfd
1
```

podman 真的好，直接 nixos 安装就可以用了之后，不像是 docker 还要一堆复杂的东西。

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
