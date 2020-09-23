# 开源EDA工具OpenRoad使用记录


<!-- vim-markdown-toc GitLab -->

- [前言](#前言)
- [解决网络问题](#解决网络问题)
    - [利用国内镜像](#利用国内镜像)
    - [proxy](#proxy)
    - [github 代理](#github-代理)
    - [让终端代理](#让终端代理)
    - [docker容器内代理](#docker容器内代理)
- [故障细节](#故障细节)
    - [yosys编译到100%然后卡住](#yosys编译到100然后卡住)
    - [TritonRoute 的构建:boost库下载过慢](#tritonroute-的构建boost库下载过慢)
    - [TritonRoute 的构建:boost编译导致死机](#tritonroute-的构建boost编译导致死机)
    - [OpenROAD的链接失效](#openroad的链接失效)
    - [OpenROAD下载cmake安装包过慢](#openroad下载cmake安装包过慢)
    - [OpenROAD安装错误版本的git](#openroad安装错误版本的git)
    - [最后一步，安装卡住不动](#最后一步安装卡住不动)
- [开始使用](#开始使用)
    - [yosys out of memory 的问题](#yosys-out-of-memory-的问题)
- [klayout 查看](#klayout-查看)

<!-- vim-markdown-toc -->

## 前言
使用OpenRoad工具主要存在两个问题，由于众所周知的原因，访问国外的网站非常的慢，导致各种软件下载总是在漫长时间的等待之后，然后失败，第二个问题是OpenROAD项目目前并不成熟，存在各种小bug，需要手动更新。

构建OpenRoad可以使用两种方法，第一种是使用docker，第二种是直接bare-metal上部署。docker的一个主要作用就是解决配置环境冲突的问题，所以docker一定是更佳的选择。

为了解决网络问题，有一种方法是租借云服务，但是我认为这种方法弊病很多:
1. 访问境外服务器延迟高，没有办法流畅的编辑，只有编辑操作非常少的时候才有用。
2. 想要获取和个人电脑，甚至本地服务器的相当的性能，价格过于昂贵。
3. 不利于长期保存和复现工作，服务器关闭，之前的工作没有了，使用snapshot无法迁移到其他的服务器厂商。

所以，如果是短期的使用，云不乏是一种选择，但是想要深入研究理解OpenRoad，还是需要选择本地部署，这与本地部署产生的网络问题，下面重点分析

## 解决网络问题

#### 利用国内镜像
利用[中科大的镜像](https://mirrors.ustc.edu.cn/help/dockerhub.html)，可以用于加速docker构建镜像的速度。

docker加速包括两个部分，第一个是第一次运行`docker run -i -t ubuntu`的时候，下载docker image会被加速，第二种是Dockerfile中间的`RUN yum install -y libffi-devel python3 tcl-devel which time`的时候，会加速下载对应的安装包。

但是对于Dockerfile中的`RUN wget https://cmake.org/files/v3.14/cmake-3.14.0-Linux-x86_64.sh`，国内镜像是无能为力的，我使用了一个略显诡谲的方法。

#### proxy
> 作为一个程序员，如果连代理问题都解决不了，那就是一种耻辱。

(为了减少不必要的麻烦，本小节出现的字符可能会插入\*，请自行过滤掉)

代理不仅仅可以解决访问本不能访问的网站，还可以给普通网站加速，包括但是不限于github。

如何实现代理，这是总是在变化的问题，在现在阶段(2020-06-01)，一个测试可行的方法是利用\*v\*2\*r\*a\*y和\*机\*场。

1. 为什么选择 \*v\*2\*r\*a\*y
    - 因为之前的各种技术**似乎**较为容易\*G\*F\*W识别，而\*v\*2\*r\*a\*y是暂时比较安全的方法。
2. 为什么选择 \*机\*场
    - 个人搭建存在非常多的技术细节，至少对于我来说，想要获取到\*机场\*的性能，很麻烦。其次价格相对贵。就加速各种软件的下载，使用\*机\*场，绰绰有余。
3. 有没有推荐的\*机\*场
    - 没有

当\*机\*场购买了解决之后，利用\*Q\*v\*r\*a\*y作为本地客户端。

#### github 代理
第一步: 执行，修改git的代理方式:
```
git config --global http.proxy http://127.0.0.1:8888
git config --global https.proxy https://127.0.0.1:8888
```
注意，端口是多少，取决于\*Q\*v\*r\*a\*y的配置。

![端口是8888](https://upload-images.jianshu.io/upload_images/9176874-24ec46c2495c320a.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

检查配置:
```
$ cat ~/.gitconfig 
[user]
    email = hubachelar@qq.com
    name = hubachelar
[http]
    proxy = http://127.0.0.1:8888
[https]
    proxy = https://127.0.0.1:8888
```

第二步: git clone 的时候，选择 https 协议。

```
git clone https://github.com/dirtycow/dirtycow.github.io.git
```

#### 让终端代理
上面的操作只是可以代理git的clone，那么对于一般的下载的代理如何解决。当想要代理的时候，执行
```
export http_proxy=http://127.0.0.1:8888
export https_proxy=http://127.0.0.1:8888
```
也可以将
```
alias setproxy='export http_proxy=http://127.0.0.1:8888 export https_proxy=http://127.0.0.1:8888'
```
放到.bashrc(如果你使用的bash)，.zshrc(如果你使用zsh)。

#### docker容器内代理
因为docker对于网络的虚拟化，docker并不会利用host的代理网络，似乎 docker build --network host 选项可以，但是测试显示并不可以。 对于网上的其他各种方法进行尝试之后，也都是失败的。

docker内不能走代理导致Dockerfile中间的执行的 wget 和 git clone 命令都没有办法成功执行。

于是学习了一下，如果你也恰好对于docker有兴趣，这是一个[参考](https://docker-curriculum.com/)，我终于理解到，Dockerfile 可以利用COPY命令，从host中间拷贝数据到docker image 中间。

有个原因非常明显，比如 Dockerfile 中间的 wget 命令，但是 Makefile 中间执行的 git clone，而且多线程执行的时候，非常难以检测为什么编译进程停止了，下面将会具体分析。


## 故障细节
上面分析了各种解决网络问题的方法，包括docker国内镜像，github代理，docker容器内步代理的绕过，下面逐个分析遇到的问题和解决方法。


#### yosys编译到100%然后卡住
经过漫长的等待之后，最后得到报错信息如下。
```
error: RPC failed; result=18, HTTP code = 200
fatal: The remote end hung up unexpectedly
fatal: early EOF
fatal: index-pack failed
+ cd abc
/bin/sh: line 4: cd: abc: No such file or directory
make: *** [Makefile:672: abc/abc-ff6758a] Error 1
The command '/bin/sh -c make PREFIX=/build -j$(nproc)' returned a non-zero code: 2
```
根据使用git的经验，这是git clone 失败的结果，因为是多线程执行的，所有线程的日志会交错的输出，所以看上去是一直在进行链接，这是一个很强的误导。
然后在Makefile的672行中间阅读相关信息，可以知道原因在编译yosys的时候，需要clone abc 这个项目。

第一步，根据Makefile的逻辑，将ABCREV设置为default，从而保证不会执行git clone
```diff
diff --git a/Makefile b/Makefile                                                
index df3c76e6..3cc12412 100644                                                 
--- a/Makefile                                                                  
+++ b/Makefile                                                                  
@@ -128,7 +128,8 @@ bumpversion:                                                
 # is just a symlink to your actual ABC working directory, as 'make mrproper'   
 # will remove the 'abc' directory and you do not want to accidentally          
 # delete your work on ABC..                                                    
-ABCREV = ff6758a                                                               
+# ABCREV = ff6758a                                                             
+ABCREV = default                                                               
 ABCPULL = 1                                                                    
 ABCURL ?= https://github.com/The-OpenROAD-Project/abc.git                      
 ABCMKARGS = CC="$(CXX)" CXX="$(CXX)" ABC_USE_LIBSTDCXX=1      
```
第二步，在`OpenROAD-flow/tools/yosys`中间将abc clone下来，因为host上代理问题已经解决，所以这是一个很快的过程。

第三步，去掉.dockerignore中间的abc,让abc可以被拷贝到docker中间。
```diff
diff --git a/.dockerignore b/.dockerignore
index cb6fdb7f..6f7b6fe1 100644   
--- a/.dockerignore               
+++ b/.dockerignore               
@@ -22,7 +22,6 @@ __pycache__     
 /coverage.info                   
 /coverage_html                   
 /Makefile.conf                   
-/abc                             
 /viz.js                          
 /yosys                           
 /yosys.exe                       
```

#### TritonRoute 的构建:boost库下载过慢
将Dockerfile中间的wget下载换成拷贝host中间已经下载好的。
```diff
diff --git a/Dockerfile b/Dockerfile                                                                     
index 3ce9b5a..22ada04 100644                                                                            
--- a/Dockerfile                                                                                         
+++ b/Dockerfile                                                                                         
@@ -25,11 +25,12 @@ RUN wget https://cmake.org/files/v3.14/cmake-3.14.0-Linux-x86_64.sh && \             
     ./cmake-3.14.0-Linux-x86_64.sh --skip-license --prefix=/usr/local                                   
                                                                                                         
 # installing boost for build dependency                                                                 
-RUN wget https://sourceforge.net/projects/boost/files/boost/1.72.0/boost_1_72_0.tar.bz2/download && \   
-    tar -xf download && \                                                                               
+# RUN wget https://sourceforge.net/projects/boost/files/boost/1.72.0/boost_1_72_0.tar.bz2/download && \ 
+COPY ./download /                                                                                       
+RUN tar -xf download && \                                                                               
     cd boost_1_72_0 && \                                                                                
     ./bootstrap.sh && \                                                                                 
-    ./b2 install --with-iostreams -j $(nproc)                                                           
+    ./b2 install --with-iostreams -j 1                                                                  
                                                                                                         
 FROM base-dependencies AS builder                                                                       
```

#### TritonRoute 的构建:boost编译导致死机
根据编译过llvm的经验，C++的程序链接过程会消耗大量的内存，所以将原先的多核并行编译切换为单线程编译。

#### OpenROAD的链接失效
安装使用链接已经过时，具体参考: https://github.com/iusrepo/announce/issues/18

OpenROAD-flow/tools/OpenROAD/Dockerfile 需要做出如下修改:
```diff
diff --git a/Dockerfile b/Dockerfile
index c0c9c7cb..71c0674a 100644
--- a/Dockerfile
+++ b/Dockerfile
@@ -3,7 +3,8 @@ LABEL maintainer="Abdelrahman Hosny <abdelrahman_hosny@brown.edu>"
 
 # Install dev and runtime dependencies
 RUN yum group install -y "Development Tools" \
-    && yum install -y https://centos7.iuscommunity.org/ius-release.rpm \
+    # && yum install -y https://centos7.iuscommunity.org/ius-release.rpm \
+    && yum install -y https://repo.ius.io/ius-release-el7.rpm \
     && yum install -y wget centos-release-scl devtoolset-8 \
     devtoolset-8-libatomic-devel tcl-devel tcl tk libstdc++ tk-devel pcre-devel \
     python36u python36u-libs python36u-devel python36u-pip && \
@@ -11,8 +12,9 @@ RUN yum group install -y "Development Tools" \
     rm -rf /var/lib/apt/lists/*
```
#### OpenROAD下载cmake安装包过慢
类似的解决办法
```diff
diff --git a/Dockerfile b/Dockerfile
index c0c9c7cb..71c0674a 100644
--- a/Dockerfile
+++ b/Dockerfile
 # Install CMake
-RUN wget https://cmake.org/files/v3.14/cmake-3.14.0-Linux-x86_64.sh && \
-    chmod +x cmake-3.14.0-Linux-x86_64.sh  && \
+# RUN wget https://cmake.org/files/v3.14/cmake-3.14.0-Linux-x86_64.sh && \
+COPY ./cmake-3.14.0-Linux-x86_64.sh /
+RUN  chmod +x cmake-3.14.0-Linux-x86_64.sh  && \
     ./cmake-3.14.0-Linux-x86_64.sh --skip-license --prefix=/usr/local && rm -rf cmake-3.14.0-Linux-x86_64.sh \
     && yum clean -y all
```
 
#### OpenROAD安装错误版本的git
类似的解决办法
```diff
@@ -22,7 +24,8 @@ RUN wget https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
     && yum clean -y all
 
 # Install git from epel
-RUN yum -y remove git && yum install -y git2u
+# RUN yum -y remove git && yum install -y git2u
+RUN yum upgrade -y git
 
 # Install SWIG
 RUN yum remove -y swig \
```

#### 最后一步，安装卡住不动
日志为:
```
Loaded plugins: fastestmirror, langpacks
```
解决办法: 查阅资料之后，似乎并不是什么严重的问题，只需要等待即可。


## 开始使用
1. source ./setup_env.sh 将三个工具添加到环境中间
2. git clone https://github.com/SI-RISCV/e200_opensource.git 获取CPU 源代码

3. 在/OpenROAD-flow/flow/designs/nangate45路径下编写e200.mk文件，其内容为:
```
export DESIGN_NAME = e203_cpu_top
export PLATFORM    = nangate45

export VERILOG_FILES = $(wildcard /OpenROAD-flow/e200_opensource/rtl/e203/core/*.v) \
                       $(wildcard /OpenROAD-flow/e200_opensource/rtl/e203/general/*.v)
export SDC_FILE      = ./designs/src/e200/e200.sdc

# These values must be multiples of placement site
# x=0.19 y=1.4
export DIE_AREA    = 0 0 2774.76 2398.2
export CORE_AREA   = 30.21 29.4 2744.55 2368.8

export CLOCK_PERIOD = 5.600
```
4. 编辑 /OpenROAD-flow/flow/Makefile，添加 `DESIGN_CONFIG = ./designs/nangate45/e200.mk`，用于指向e200.mk。
5. make

#### yosys out of memory 的问题
将 /OpenROAD-flow/e200_opensource/rtl/e203/core/e203_cpu_top.v 中间的sram 部分注释掉

## klayout 查看

将编译好的gds 文件从 docker 拷贝出来:
```
sudo docker cp bda21bdf7399://OpenROAD-flow/flow/results/nangate45/e203_cpu_top/6_final.gds .
```

编译安装klayout
git clone https://github.com/KLayout/klayout.git
./build.sh -qt5
需要很长时间

运行，由于没有安装，执行的时候需要指明动态链接库:
cd klayout/bin-release
./klayout -L./

结果:
![Screen Capture_select-area_20200528170553.png](https://upload-images.jianshu.io/upload_images/9176874-8c016fab064fc840.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)
