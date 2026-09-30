> 以*Data Structure and Algorithm Analysis in C* 为基础总结常用的数据结构和算法

https://nitsri.asia/Department/Computer%20Science%20_%20Engineering/DS.pdf

## sort

1. insert sort : insert sort makes sure 1 -- P is sorted
2. bubble sort : keep find the smallest elements
3. bucket sort :
4. radix sort
	1. 从低位排序
	2. 需要含有记录每一个维度大小的数组

## list
### skip list
1. define a level k node to be a node that has k pointers
2. ith pointer in any level k node(k >= i) points to the next node at least i levels
(也就说指针的指向线段总是水平的)
3. the easiest way to determining the level of a node is to flip a coin until a head occur

## tree
1. 搜索tree 的三种方法各自的特点

1. 删除: 总是需要找到一个至少一个子节点为空, 如果采用一下方法删除,会导致整棵树向左边倾斜
```
 private Node delete(Node x, Key key) {
        if (x == null) return null;

        int cmp = key.compareTo(x.key);
        if      (cmp < 0) x.left  = delete(x.left,  key);
        else if (cmp > 0) x.right = delete(x.right, key);
        else {
            if (x.right == null) return x.left;
            if (x.left  == null) return x.right;
            Node t = x;
            x = min(t.right);
            x.right = deleteMin(t.right);
            x.left = t.left;
        }
        x.size = size(x.left) + size(x.right) + 1;
        return x;
    }
```


### Balanced Tree


### AVL Tree
An AVL tree is a binary search tree which has the following properties:
1. The sub-trees of every node differ in height by at most one.
2. Every sub-tree is an AVL tree.
single rotation and double rotation


### red black tree
1. Every node has a color either red or black.
2. Root of tree is always black.
3. There are no two adjacent red nodes (A red node cannot have a red parent or red child).
4. Every path from root to a NULL node has same number of black nodes.

The AVL trees are more balanced compared to Red Black Trees, but they may cause more rotations
during insertion and deletion


### splay tree
1. in many application, when a node is accessed, it is likely to be access again in the near future
2. splay tree don't require the maintenance of height or balance information
3. down-top rotation need over head and too much special case **其实并不知道是哪一些special case**.
in top-down case,keep split the tree and assemble them at last


### B-tree
1. the root is either a leaf or has between 2 and M children
2. all non-leaf nodes(except the root) have between 向下取整(M / 2) and M children
3. all leaves are at the same depth
### top down splay tree

### AA-tree

### treap

### k-d tree

### B-tree

## heap

### pairing heap

## lca


## 查漏补缺一下吧
知道。Mark Allen Weiss 的《Data Structures and Algorithm Analysis in C》(数据结构与算法分析——C 语言描述),
常见的还有 Java 版和 C++ 版(《Data Structures and Algorithm Analysis in C++》)。

特点:

- 主线:先讲算法分析方法(渐进记号 O/Ω/Θ、增长率、递归式求解),再把数据结构当作"实现 + 复杂度分析"来推导,而不是单纯罗列 API。
- 目录大致:
    - 引论、算法分析(数学基础、递归式)
    - 表、栈、队列(链表实现、游标实现)
    - 树(AVL、伸展树、B 树)、散列(开放寻址、分离链接)
    - 优先队列(二叉堆、左式堆、斜堆、二项队列)
    - 排序(插入/希尔/堆/归并/快排、桶排序、外部排序)
    - 不相交集(union-find,按秩合并 + 路径压缩)
    - 图论算法(拓扑排序、最短路 Dijkstra/Bellman-Ford、网络流、最小生成树)
    - 算法设计技巧(贪心、分治、动态规划、随机化、回溯)
    - 摊还分析(二项队列、斜堆、斐波那契堆)
    - 高级数据结构与算法(自顶向下伸展树、k-d 树、配对堆、后缀数组、最大流)
- 几个著名特色:并查集那章对路径压缩的分析、左式堆/斜堆、摊还分析讲得比多数教材细;C 版会直接写可编译的实现,比 Java 版略"底层"。
- 常见坑:C 版的代码偏教学习惯(比如 typedef 出的类型抽象层、部分错误处理简化),直接抄到生产代码不合适;某些中文译本的符号和排版一般。

如果你想对照复习或找某个具体主题(比如并查集复杂度、AVL 旋转、Kruskal/Prim 实现)的细节,告诉我具体点,我可以按书里的思路讲清楚。

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
