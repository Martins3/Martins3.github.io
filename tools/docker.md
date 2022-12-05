# 注意
https://support.getjoan.com/hc/en-us/articles/360008889859-How-to-change-the-Docker-default-subnet-IP-address

由于 Loongson 和 docker 的冲突，在此处将 docker 的 briage ip 进行了修改，目前可以上网，但是对于 docker 造成的结果是不知道的。


[lazydocker](https://github.com/jesseduffield/lazydocker)
- https://nystudio107.com/blog/dock-life-using-docker-for-all-the-things

![](https://nystudio107-ems2qegf7x6qiqq.netdna-ssl.com/img/blog/_1200x409_crop_center-center_100_line/anatomy-of-a-docker-alias.png.webp)

## TODO
https://fuckcloudnative.io/posts/docker-images-part1-reducing-image-size/

https://unixism.net/2020/06/containers-the-hard-way-gocker-a-mini-docker-written-in-go/ : 使用 1000 行实现 docker，同时可以练习一下 go

https://github.com/yeasy/docker_practice : 一本书

https://github.com/shuveb/containers-the-hard-way : docker inside

## 文摘
- https://devopsdirective.com/posts/2021/04/tiny-container-image/ : 如何制作最小的 docker image

## [awesome](https://github.com/veggiemonk/awesome-docker)

## dockerfile 的原理是什么 ?
1. docke 中间的各种 run 可不可以进入到镜像中间运行
2. docker run --net = host 的含义是什么 ?

## 其他小问题
docker 代理

## 资源
https://github.com/jesseduffield/lazydocker : 非常有意思，可以对于所有 docker 的监控


- docker pull archlinux
- docker run -it archlinux
- docker start container_id -i
- docker exec -it container_id zsh
- docker attach : same input and out

https://stackoverflow.com/questions/19585028/i-lose-my-data-when-the-container-exits

## ctop

## mistake and todo
1. 创建docker默认使用 root，但是 root 非常的不清真，应该创建用户，之后再改用户的角度使用

## 使用 aliyun 加速
```json
{
  "registry-mirrors": [
    "https://hub-mirror.c.163.com",
    "https://mirror.baidubce.com"
  ]
,
"default-address-pools":
 [
 {"base":"10.10.0.0/16","size":24}
 ]
}
```
https://cr.console.aliyun.com/cn-hangzhou/instances/mirrors 修改为:

```sh
sudo tee /etc/docker/daemon.json <<-'EOF'
{
  "registry-mirrors": ["https://123213413421324.mirror.aliyuncs.com"]
}
EOF
sudo systemctl daemon-reload
sudo systemctl restart docker
```
