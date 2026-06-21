# 命运的织机: 各种 scheduler 杂谈
<!-- 382ede37-4ae2-4713-8158-b3b4d2550838 -->

## scheduler 解决什么问题

### 负载均衡

> 《孟子·梁惠王上》
> 梁惠王曰："寡人之于国也，尽心焉耳矣。河内凶，则移其民于河东，移其粟于河内；
> 河东凶亦然。察邻国之政，无如寡人之用心者。邻国之民不加少，寡人之民不加多，何也？"

### 优先级控制

### 请求合并

## CPU scheduler
这就太多了

## GPU scheduler
drm scheduler

## Block device
上课内容

## Network device
上课内容

## K8s
有趣，在大公司应该是一组人在做吧

## human scheduler in AI Era
<!-- 3c5b4e26-9988-454a-b843-40ae05b3a949 -->

尽量只做一个事情，不要自己骗自己，由于一个事情要超时了，然后去做另外的一个事情。
就像是所有的事情都在推进一样。

1. 有的工作是着急的
2. 有的工作是机器在跑的，只需要点一下
3. 有的工作很快就可以解决，有的需要一些时间
4. 上下文的切换的时间不同，有的需要打开很多文件，终端，浏览器的 tab
5. 有的工作需要其他人的配合

其实是遇到过问题，但是
一个事情，如果需要很长的时间运行，人的 cache 很快就会流失的；

1. 没有压力的推进不同的事情。
2. 并发思考，但是有并发的提交，但是仅仅思考一件事情。当一件事情完成后，之后并发看看所有的可以快速继续提交的事情。
3. 写一个列表，记录现在正在做什么?
4. 注意休息，放松，高并发是主动选择的结果，而不是压力导致的。

让 ai 并发，而不是让人逃避工作，如果可以让 ai 完成，那么就立刻交给 ai 来完成。

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
