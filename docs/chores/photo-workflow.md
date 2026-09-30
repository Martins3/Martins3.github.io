# 摄影摄像环境
<!-- c30488f3-807e-4f91-9830-196e3cadcd0d -->

## 软件 : Immich
经过调研，那么就是这个项目就是最好的了:
https://github.com/immich-app/immich


利用 [Docker Compose](https://docs.docker.com/compose/) 部署 [Immich](https://immich.app/)。

官方安装文档：[https://docs.immich.app/install/docker-compose](https://docs.immich.app/install/docker-compose)

### 快速部署

```bash
mkdir -p ~/immich-app
cd ~/immich-app

# 下载官方 compose 文件和示例 env
curl -L https://github.com/immich-app/immich/releases/latest/download/docker-compose.yml -o docker-compose.yml
curl -L https://github.com/immich-app/immich/releases/latest/download/example.env -o .env

# 编辑 .env，修改上传目录、数据库目录、密码、版本
cat > .env <<'EOF'
UPLOAD_LOCATION=/home/martins3/immich-app/library
DB_DATA_LOCATION=/home/martins3/immich-app/postgres
IMMICH_VERSION=v2.6.3
DB_PASSWORD=immichpassword123
DB_USERNAME=postgres
DB_DATABASE_NAME=immich
EOF

# v2.6.3 使用 ankane/pgvector 作为数据库
# latest 官方 compose 默认使用 vectorchord（v3 引入）
sed -i 's|image: ghcr.io/immich-app/postgres:14-vectorchord0.4.3-pgvectors0.2.0@sha256:.*|image: ankane/pgvector:latest|' docker-compose.yml
sed -i '/POSTGRES_INITDB_ARGS/d' docker-compose.yml

# 启动
sudo docker-compose up -d
```
第一次打开会进入管理员注册页面，按提示创建账号即可开始使用。


### 常用命令

```bash
cd ~/immich-app

# 查看运行状态
sudo docker-compose ps

# 查看日志
sudo docker-compose logs -f immich-server
sudo docker-compose logs -f immich-machine-learning
sudo docker-compose logs -f database

# 重启
sudo docker-compose restart

# 完全停止并删除容器（数据卷会保留）
sudo docker-compose down

# 停止并删除容器和数据卷（会丢失照片和数据库，慎用）
sudo docker-compose down -v

# 升级 Immich，假设要升级到 `v2.7.0`：
sed -i 's/IMMICH_VERSION=.*/IMMICH_VERSION=v2.7.0/' .env
sudo docker-compose pull
sudo docker-compose up -d
```

### 其他竞品:
- https://github.com/photoprism/photoprism
- Lychee
- https://github.com/besscroft/PicImpact

## 硬件
### 云台相机调研

- https://www.bilibili.com/video/BV1Luj969EWg : 广告不说的缺点，我来扒 | 影石luna 大疆pocket4pro 40个功能【差异】篇 | 个人体验
- https://www.bilibili.com/video/BV1BGjZ6MEdf : 大疆pocket4P说点很多博主不敢提的
- https://www.bilibili.com/video/BV1ghJg6hEWV : 大疆Pocket 4P，到底 Pro 在哪？
- https://www.bilibili.com/video/BV1ZLJg6rEbJ : 800块加个镜头，值吗？大疆Pocket 4P体验
- https://www.bilibili.com/video/BV1DgdhBGEq2 : 终于来了！大疆Pocket 4上手

综合考虑，还是 dji pocket4


### 照片打印机调研
<!-- 3a5ef945-88de-4b24-a291-88013e3e2e1e -->

**照片打印中，我会这样选：画质优先，选热升华；
特别在意小巧、随手打印和贴手账，考虑 ZINK。** 不过，**热升华也能打印贴纸**，背胶并不是 ZINK 独有的优势。:chatgpt-content-reference{index="0"}

## 1. 本质区别：颜色材料放在哪里？

**ZINK：颜色材料已经在相纸里，加热使它显色。**
ZINK 是 *Zero Ink*，意思是不需要独立的墨盒、色带或碳粉。专用相纸内有受热显色的材料，打印时直接在纸上形成图像，一次走纸即可完成。所谓“无墨”，并不是没有显色材料，而是**把它预先做进了纸里**。:chatgpt-content-reference{index="1"}

**热升华：颜色材料在色带上，加热把染料转移到相纸里。**
照片打印机使用专用色带和相纸，分别转移黄、品红、青三种染料，通过控制热量控制染料转移量，形成不同浓淡的颜色；常见系统最后还会加一层透明保护涂层。:chatgpt-content-reference{index="2"}

可以记成：**ZINK 是“让纸自己显色”，热升华是“把色带上的颜色转移到纸上”。**

## 2. 实际使用有什么区别？

| 对比项 | ZINK | 热升华 |
|---|---|---|
| **耗材** | 专用 ZINK 相纸，不需要独立色带 | 专用相纸＋色带，常按配套张数出售 |
| **机器结构** | 省去色带机构，有利于做得小巧 | 需要色带及相应传动机构 |
| **打印过程** | 一次走纸完成彩色图像 | 分色打印，再加保护涂层 |
| **画质取向** | 更适合便携分享、手账等用途；重视还原度时建议先看实样 | 平滑的色调渐变是强项，更适合作为照片画质优先的选择 |
| **耐日常接触** | 专用纸本身可以防泼水、抗污，并非一碰水就坏 | 保护涂层有助于抵抗水和指纹 |
| **贴纸** | 背胶相纸很常见 | 也有贴纸耗材，取决于机型 |

以上原理、耗材及耐用性区别可见 ZINK 和热升华厂商的技术说明；具体尺寸、速度和打印效果仍应按机型比较。:chatgpt-content-reference{index="3"}

## 3. 三个容易误判的地方

**“无墨”不等于耗材便宜。**
ZINK 的显色材料成本包含在相纸里；热升华则要计算相纸与色带的总成本。比较时应该看：

> **每张成本＝配套耗材总价÷实际可打印张数**

同时要比较相同照片尺寸，不能只看“每张多少钱”，也不能只凭有没有色带判断贵贱。:chatgpt-content-reference{index="4"}

**不要只看 dpi 判断画质。**
dpi 反映打印点的密度，不等于颜色准确度或渐变质量。例如佳能 SELPHY CP1500 标称为 **300×300 dpi、每种颜色 256 级浓淡**。因此，除了分辨率，也应看肤色、灰阶和暗部的实际打印效果。:chatgpt-content-reference{index="5"}

**“能保存 100 年”是有条件的，不是永久不褪色。**
例如佳能对部分热升华照片的“100 年”说明，基于**相册保存、23°C、50% 相对湿度条件下的加速测试**，不能理解为贴在窗边晒太阳也能保持 100 年。ZINK 的防水、抗污，也不能直接等同于长期抗褪色。:chatgpt-content-reference{index="6"}

**我的建议：没有很强的口袋便携需求，优先考虑热升华；选择 ZINK，主要应该是为了它省去色带、小巧方便，而不是因为“无墨”听起来更先进。**

TODO :
1. 差别有多少?
2. 原理
3. 相纸价格

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
