#!/bin/bash

image="library/nginx:1.19"

# 比如要获取镜像的 tag 常用的是 echo 然后 awk/cut 的方式

# 可以直接使用 bash 内置的变量替换功能，截取特定字符串
image_name=${image%:*}
image_tag=${image#*:}
image_repo=${image%/*}
echo "${image_name}"
echo "${image_tag}"
echo "${image_repo}"
