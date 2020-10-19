## TODO
https://fuckcloudnative.io/posts/docker-images-part1-reducing-image-size/

https://unixism.net/2020/06/containers-the-hard-way-gocker-a-mini-docker-written-in-go/ : 使用 1000 行实现 docker，同时可以练习一下 go

https://github.com/yeasy/docker_practice : 一本书


https://github.com/shuveb/containers-the-hard-way : docker inside

## [awesome](https://github.com/veggiemonk/awesome-docker)



## dockerfile 的原理是什么 ?
1. docke 中间的各种 run 可不可以进入到镜像中间运行
2. docker run --net = host 的含义是什么 ?

## 其他小问题
docker 代理

## 资源
https://github.com/jesseduffield/lazydocker : 非常有意思，可以对于所有 docker 的监控



## Get Off Stupid apt

docker pull archlinux

docker run -it archlinux

docker start container_id -i

docker exec -it container_id zsh

docker attach : same input and out

https://stackoverflow.com/questions/19585028/i-lose-my-data-when-the-container-exits


## mistake and todo
1. 创建docker默认使用 root，但是 root 非常的不清真，应该创建用户，之后再改用户的角度使用
2. 
