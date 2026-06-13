1. Transformer为何使用多头注意力机制？（为什么不使用一个头）
2. Transformer为什么Q和K使用不同的权重矩阵生成，为何不能使用同一个值进行自身的点乘？ （注意和第一个问题的区别）
14. 简单描述一下Transformer中的前馈神经网络？使用了什么激活函数？相关优缺点？
2. Q/K/V 的 weight 矩阵是所有层共享一套，还是每层都有单独的一套？
3. Transformer计算attention的时候为何选择点乘而不是加法？两者计算复杂度和效果上有什么区别？
4. 为什么在进行softmax之前需要对attention进行scaled（为什么除以dk的平方根），并使用公式推导进行讲解
5. 在计算attention score的时候如何对padding做mask操作？
6. 为什么在进行多头注意力的时候需要对每个head进行降维？（可以参考上面一个问题）
9. 简单介绍一下Transformer的位置编码？有什么意义和优缺点？
10. 你还了解哪些关于位置编码的技术，各自的优缺点是什么？
11. 简单讲一下Transformer中的残差结构以及意义。
12. 为什么transformer块使用LayerNorm而不是BatchNorm？LayerNorm 在Transformer的位置是哪里？
13. 简答讲一下BatchNorm技术，以及它的优缺点。
8. 为何在获取输入词向量之后需要对矩阵乘以embedding size的开方？意义是什么？

7. 大概讲一下Transformer的Encoder模块？
15. Encoder端和Decoder端是如何进行交互的？（在这里可以问一下关于seq2seq的attention知识）
16. Decoder阶段的多头自注意力和encoder的多头自注意力有什么区别？（为什么需要decoder自注意力需要进行 sequence mask)
17. Transformer的并行化提现在哪个地方？Decoder端可以做并行化吗？
19. Transformer训练的时候学习率是如何设定的？Dropout是如何设定的，位置在哪里？Dropout 在测试的需要有什么需要注意的吗？
20. 解码端的残差结构有没有把后续未被看见的mask信息添加进来，造成信息的泄露。

## vllm 问题的理解
1. 如果 kv cache 不命中，需要从头计算吗?

## 更多的参考资料
- Stanford CS224N: Natural Language Processing with Deep Learning.
- The Illustrated Transformer.
	- https://jalammar.github.io/illustrated-transformer/ 这个真的是不错的
	- https://www.llm-book.com/ : 这个书看看吗?

## 还是需要继续看看类似这个的东西了
https://zhuanlan.zhihu.com/p/691038809

## douyin 中的关于 kv cache 需要多大显存的，也是需要看看了

## 所以，什么是 decode only 的架构?

## flash attension 中就是这个？
FlashAttention: Fast and Memory-Efficient Exact Attention with IO-Awareness
https://github.com/fla-org/flash-linear-attention


## 这几个东西看看
https://zhuanlan.zhihu.com/p/46990010
https://zhuanlan.zhihu.com/p/311156298
https://zhuanlan.zhihu.com/p/638884759
https://zhuanlan.zhihu.com/p/569527564

直观解释注意力机制，Transformer的核心
https://www.bilibili.com/video/BV1TZ421j7Ke
不容易理解，似乎直接问题

## 各种公式的补充
https://mp.weixin.qq.com/s/EHXBbN-G5X05rKTo1GpQkA

https://zhuanlan.zhihu.com/p/633431594

## qkv ，为什么存在这些东西
https://news.ycombinator.com/item?id=48405931

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
