## TODO

ref : https://leetcode.com/discuss/interview-question/753236/List-of-graph-algorithms-for-coding-interview

- [ ] Union Find: Tonnes of problems on Leetcode
Single/Multi-source Shortest Path: Dijkstra, Bellman, Floyd-Warshall algorithm. Leetcode has good amount of problems on this topic. Few are following
Minimum Spanning Trees: Prim's and Kruskal's algorithm.
https://leetcode.com/problems/optimize-water-distribution-in-a-village/

- [ ] 332
`A*` Search
- [ ] 773

Max-Flow, Min-Cut
- [ ] https://leetcode.com/problems/maximum-students-taking-exam/

Articulation points and bridges
- [ ] https://leetcode.com/problems/critical-connections-in-a-network/

MST (minimum spanning tree), which can be built using Kruskal's or Prim's
- [ ] 1489 https://leetcode.com/problems/find-critical-and-pseudo-critical-edges-in-minimum-spanning-tree/

Floyd-Warshall
- [ ] 1334 https://leetcode.com/problems/find-the-city-with-the-smallest-number-of-neighbors-at-a-threshold-distance/
- [ ] 399 https://leetcode.com/problems/evaluate-division/
- [ ] 787 https://leetcode.com/problems/cheapest-flights-within-k-stops/
- [ ] 1462 https://leetcode.com/problems/course-schedule-iv/
- [ ] 1617 https://leetcode.com/problems/count-subtrees-with-max-distance-between-cities/


## 图论算法
> 基于 https://github.com/TheAlgorithms/C-Plus-Plus

有权边的邻接表的构建方法:
```cpp
void addEdge(std::vector<std::vector<std::pair<int, int>>> *adj, int u, int v,
             int w) {
    (*adj)[u - 1].push_back(std::make_pair(v - 1, w));
    // (*adj)[v - 1].push_back(std::make_pair(u - 1, w));
}
```

# trie


# 最短路算法

## Dijkstra
distance 数组
visited 数组 : 从 priority_queue 中间出来的点需要被标记一下
priority_queue : 用于选择当前离 start 最近的节点

到达 start 的距离被刷新，那么就加入到 priority_queue 中间

代码细节:
1. [std::pair](https://en.cppreference.com/w/cpp/utility/pair/operator_cmp) 和 [std::greater](https://stackoverflow.com/questions/12147566/c-greater-function-object-definition)

## Bellman-Ford
算法：所有的边来依次更新距离，更新次数为 size(vertex) - 1
动态规划： 每一步计算都是保证第 i 步可以到达的点是最优的

[Bellman–Ford 可以处理 edge weight 是负数的情况](https://stackoverflow.com/questions/19482317/bellman-ford-vs-dijkstra-under-what-circumstances-is-bellman-ford-better)

当然，Bellman-Ford 无法处理 negative 环路，想要检查出来 negative 环路，就再跑一次
## Floyed-Warshall
注意矩阵的对称和中心的位置。
for k for i for j if dis[i][k] +  dis[k][j] < dis[i][j]

- [ ] 这里的 i, k, j 可以随意嵌套吗 ?
  - 并不知道，但是 k 在最外面必然是正确的，对于每个 k 的循环计算，都可以计算出来和 k 相连定点之间的最短数值。

# dfs
visted 数组

递归搜索，比如对于一棵树找到从 head 到某个点的求和等于一个数情况，node 进入的时候 sum += val, node 退出的时候 sum -= val

## 查找环路
inStack 数组

## dfs with stack
visted 数组
queue

[使用 stack 访问的一个直觉写法](https://11011110.github.io/blog/2013/12/17/stack-based-graph-traversal.html)
```py
def stack_traversal(G,s):
    visited = {s}
    stack = [s]
    while stack:
        v = stack.pop()
        for w in G[v]:
            if w not in visited:
                visited.add(w)
                stack.append(w)
```
上面这一种显然不行，就思考二叉树的情况，第一个元素上来就
将其两个 child 都标记为 visited

```py
def dfs2(G,s):
    visited = set()
    stack = [s]
    while stack:
        v = stack.pop()
        if v not in visited:
            traversed_path.push_back(v)
            visited.add(v)
            for w in G[v]:
                stack.append(w)
```
将 stack 的 top 取出来，然后 push neighbor 的元素，
dfs 唯一的问题在于，其需要检查 pop 出来的是否 visited 过了。

C-Plus-Plus/graph/depth_first_search_with_stack.cpp 实现的算法是非常蠢的，应该修改一下。

# bfs
visted 数组
queue

# 最小生成树

## Prim
- 类似于 Dijkstra，不需要 distance 数组
  - 随便找一点加入到 priority_queue 中间, 从而找到最小的边
  - 从 priority_queue 出来的不要和现在的边构成回路, 也即是找最小边的时候，要看另一端的点是否 visted 过了

和 Dijkstra 的区别，一旦一个点加入，需要刷新周围的数值，并且将这些刷新放到 priority_queue 中

## Kruskal
使用 union-find 实现

将所有的节点从小到大排序，当 edge 的两个端点没有被 union 在一起，那么就是被选中的

## 查找 cut vertex 和 bridge
cut vertex 和 bridge 表示这个点/边不在环路中间, 所以去掉之后，这个图的连通性不改变

算法: 每一个节点维持两个变量 a, b。 dfs 开始访问的时候， a = b = count ++, dfs 结束的时候 b 等于所有点的最小值

cut vertex : b 大于等于 a。
cut edge ： 两个端点 b 差值为 1

cut vertex 的根节点不需要特殊考虑一下
```c
// (1) u is root of DFS tree and has two or more chilren.
if (parent[u] == NIL && children > 1)
  ap[u] = true;
```

- [x] 如果是存在多个 cut vertex 怎么办? (这是对于所有的点进行询问的)

## Euler Circuits
所有的边仅仅访问一次

对于无向图:
1. 所有点度数是偶数， 构成回路
2. 仅仅两个点的度数是奇数， 构成路径


对于有方向图:
1. 所有点上的出度和入度全部相同
2. 如果不同，需要成组对应

算法: Hierholzer's algorithm
首先进行 dfs，当无路可走的时候，那么 dfs 返回，直到重新可以 dfs 了

## Topologic sort
topo 排序的前提 : 如果含有 `A->B` 的边，  那么 dfs 的时候 B 必定更早的结束

当一个点结束访问时候放入 stack 中间。

- 对于无向图，这个没有什么意义
- 对于环路，其实虽然可以输出结果，但是不符合语义要求

- [Hamiltonian_path_problem](https://en.wikipedia.org/wiki/Hamiltonian_path_problem) 目前不存在很好的算法

## SCC(Strongly Connected Component)
定义 : 从任何一个点可以到达任何一个点

- 对于无向图，这个没有什么意义

算法:
- 对于所有点进行拓扑排序操作， 得到一个标记顺序 a -> b。
- 构建反图
- 按照结束的顺序进行dfs, 每次得到的都是一个SCC, 首先访问 a, 只要访问到就是了

证明 :
1. a b 在同一个， 那么必定通过该点的算法，可以归到一个CC中间
2. 如果a b 不在，在反图上，从 a 开始 dfs 一定是找不到 b 的。
  - 使用反证法 : a b 不在一个 SCC 上，在反图上, 如果从 a 开始 dfs 可以找到 b , 那么在正图上，必然存在 b -> a, 但是这样 a 就不应该排在前面


## Shortest Path Faster Algorithm

## summary
1. Dijkstra's algorithm solves the single-source shortest path problem, no negative path possible
O(E + V(logV))
2. Bellman–Ford algorithm solves the single-source problem if edge weights may be negative
O(V*E)
对于所有的边, 对于周围的两个顶点进行收缩
3. A* search algorithm solves for single pair shortest path using heuristics to try to speed up the search.
4. Floyd–Warshall algorithm solves all pairs shortest paths.
O(V\*V\*V)
5. Johnson's algorithm solves all pairs shortest paths, and may be faster than Floyd–Warshall on sparse graphs.
6. Viterbi algorithm solves the shortest stochastic path problem with an additional probabilistic weight on each node


*Directed acyclic graph*  is a finite directed graph with no directed cycles
*Longest path problem* has a linear time solution for directed acyclic graphs, which has important applications in finding the critical path in scheduling problems. algorithm is as below:

```
init all distance as NEGATIVE_INF
make the topological sort of the graph
dis[s] = 0
for u in graph vertex topological order start with s:
    for v in neighbors of u:
        dis[v] = max(div[v], dis[u] + weight(u, v))
# 依次对于所有的点进行 点收缩的操作
# 既没有思考如何证明, 也没有思考为什么对于通用的算法是 NP
```

a *biconnected graph* is a connected and "nonseparable" graph, meaning that if any one vertex were to be removed, the graph will remain connected.
Therefore a biconnected graph has no articulation vertices.The property of being 2-connected is equivalent to biconnectivity, with the caveat that the
complete graph of two vertices is sometimes regarded as biconnected but not 2-connected.This property is especially useful in maintaining a graph with
a two-fold redundancy, to prevent disconnection upon the removal of a single edge (or
connection).


a *biconnected component* (also known as a block or 2-connected component) is a maximal biconnected subgraph
```
The idea is to run a depth-first search while maintaining the following information:
    1. the depth of each vertex in the depth-first-search tree (once it gets visited), and
    2. for each vertex v, the lowest depth of neighbors of all descendants(including the already visited) of v (including v itself) in the depth-first-search tree, called the lowPoint
The key fact is that a nonRoot vertex v is a cut vertex (or articulation point) separating two biconnected components if and only if there is a child y of v such that lowPoint(y) ≥ depth(v). lowPoint(y) < depth(v) manifest that y is connected to parent of v

GetArticulationPoints(i, d)
    visited[i] = true
    depth[i] = d
    low[i] = d
    childCount = 0
    isArticulation = false
    for each ni in adj[i]
        if not visited[ni]
            parent[ni] = i
            GetArticulationPoints(ni, d + 1)
            childCount = childCount + 1
            if low[ni] >= depth[i]
                isArticulation = true
            low[i] = Min(low[i], low[ni])
        else if ni <> parent[i]
            low[i] = Min(low[i], depth[ni])
    if (parent[i] <> null and isArticulation) or (parent[i] == null and childCount > 1)
        Output i as articulation point
```

*snake and ladder problem* The idea is to consider the given snake and ladder board as a directed graph with number of vertices equal to the number of cells in the board. The problem reduces to finding the shortest path in a graph. Every vertex of the graph has an edge to next six vertices if next 6 vertices do not have a snake or ladder. If any of the next six vertices has a snake or ladder, then the edge from current vertex goes to the top of the ladder or tail of the snake. Since all edges are of equal weight, we can efficiently find shortest path using Breadth First Search of the graph.

a *bipartite graph* (or bigraph) is a graph whose vertices can be divided into two disjoint and independent sets U and V such that every edge connects a vertex in U to one in V

*Boruvka’s algorithm*
```
1) Input is a connected, weighted and directed graph.
2) Initialize all vertices as individual components (or sets).
3) Initialize MST as empty.
4) While there are more than one components, do following
   for each component.
      a)  Find the closest weight edge that connects this
          component to any other component.
      b)  Add this closest edge to MST if not already added.
5) Return MST.

# 不是很清楚和 Kruska 算法有什么区别啊!
```
*Johnson’s algorithm*
The idea of Johnson’s algorithm is to re-weight all edges and make them all positive, then apply Dijkstra’s algorithm for every vertex.
Let the weight assigned to vertex u be h[u]. We reweight edges using vertex weights
he great thing about this reweighting is, all set of paths between any two vertices are increased by same amount and all negative weights become non-negative.
```
Algorithm:
1) Let the given graph be G. Add a new vertex s to the graph, add edges from new vertex to all vertices of G. Let the modified graph be G’.

2) Run Bellman-Ford algorithm on G’ with s as source. Let the distances calculated by Bellman-Ford be h[0], h[1], .. h[V-1]. If we find a negative weight cycle, then return. Note that the negative weight cycle cannot be created by new vertex s as there is no edge to s. All edges are from s.

3) Reweight the edges of original graph. For each edge (u, v), assign the new weight as “original weight + h[u] – h[v]”.

4) Remove the added vertex s and run Dijkstra’s algorithm for every vertex.
```

*Shortest Path in Directed Acyclic Graph* can be O(V + E) that is better than Dijkstra's O(V * log(V) + E), the algorithm is same as longest path problem

*Check if a graph is strongly connected* for undirected it's easy, for directed, if every node can be reached from a vertex v, and every node can reach v, then
the graph is strongly connected.

*bridge* The condition for an edge (u, v) to be a bridge is, “low[v] > disc[u]”.

*Biconnected graph* without cut vertex

*Euler Cycle*
An undirected graph has Euler cycle if following two conditions are true.
1. All vertices with non-zero degree are connected.
2. All vertices have even degree.

*Euler Path*
An undirected graph has Euler Path if following two conditions are true.
1. All vertices with non-zero degree are connected.
2.  If zero or two vertices have odd degree and all other vertices have even degree. Note that only one vertex with odd degree is not possible in an undirected graph (sum of all degrees is always even in an undirected graph)

*Hierholzer algorithm*
Hierholzer 1873 paper provides a different method for finding Euler cycles that is more efficient than Fleury's algorithm:
Choose any starting vertex v, and follow a trail of edges from that vertex until returning to v. It is not possible to get stuck at any vertex other than v, because the even degree of all vertices ensures that, when the trail enters another vertex w there must be an unused edge leaving w. The tour formed in this way is a closed tour, but may not cover all the vertices and edges of the initial graph.
As long as there exists a vertex u that belongs to the current tour but that has adjacent edges not part of the tour, start another trail from u, following unused edges until returning to u, and join the tour formed in this way to the previous tour.
By using a data structure such as a doubly linked list to maintain the set of unused edges incident to each vertex, to maintain the list of vertices on the current tour that have unused edges, and to maintain the tour itself, the individual operations of the algorithm (finding unused edges exiting each vertex, finding a new starting vertex for a tour, and connecting two tours that share a vertex) may be performed in constant time each, so the overall algorithm takes linear time, O(|E|)

*Tarjan’s Algorithm to find SCC* SCC means Strongly Connected Components, [其他的方法](http://www.geeksforgeeks.org/strongly-connected-components/)
__as the ruler , not yet implemented__


*graph matching*


*Network flow*

[definitions](https://en.wikipedia.org/wiki/Graph_(discrete_mathematics))

# Maximum flow

## *Ford-Fulkerson Algorithm*:
1. augment path from source to sink:
    1. non-full forward edges
    2. non-empty backward edges
2. residual graph : that it can happen that a flow from v to u is allowed in the residual network, though disallowed in the original network
3. residual capacity
4. minimal cut

how to do:
1. find an augmenting path
2. compute the bottleneck capacity
3. augment each edge and the total flow

只要保证所有的CF都是大于0的路径存在就是可以的:
cf 根据当前的被当前的 f 确定, 表示为当前的剩余的flow 的数量
找到之后的: 确定整个路径的最小的flow 为 cfp, 修改图形上面的flow 的变化量
贪心算法: 只要含有路径都是含有存量的时候, 就是继续进行

notice that it can happen that a flow from v to u is allowed in the residual network, though disallowed in the original network: 也即是可以通过查找消减之前已经得到的增加的量进行消除的.

When no more paths in step 2 can be found, s will not be able to reach t in the residual network
只要含有余量的时候, 两个点就是连通的, 那么问题就是转换为两个点连通 的检测
深度优先 或者 广度有限:

## Find maximum number of edge disjoint paths between two vertices
1. disjoint means that path share no edge
2. 设置所有的边的capacity 都是的常数, 然后跑 最大流

## Find minimum s-t cut in a flow network
1. Max-flow min-cut theorem:  in a flow network, the amount of maximum flow is equal to capacity of the minimum cut [prove](https://en.wikipedia.org/wiki/Max-flow_min-cut_theorem)
2. 实现:
    1. 对于当前的图 run Maximum flow 得到 最后的residual graph
    2. 对于 residual graph, 对于的source 可以到达的点 到 source 不可以到达的 edge, 就是目标 的 edge

3. 理解:
    1. 分割双方 也就是 限制最大流 的关键

## Maximum Bipartite Matching
1. A maximum matching is a matching of maximum size (maximum number of edges).
2. 转化的方法: 标记 bipartite 的两个部分为L 和 R, 首先添加 source 和 end, source 指向所有L, 所有的R 指向 R 部分,
最大流的中间的一条路径必定和 一个匹配是一一对应的

#### Channel Assignment Problem
*todo*

# Implementation
1. use queue and stack
    1. three stage: already finished, not reached, in container
    2. queue: finish the operation and adding the neighbor at the same time
    3. stack: until all the neighbor finished!
    4. **使用函数的递归可以轻松的实现 前序 还是 后序的访问, 但是stack 的实现需要使用一些技巧**
    5. **尚且不是很清楚什么是非要使用后序遍历的 dfs**

2. use function
    1. add the visited array parameter(this is just a shallow copy)
    2. make visited array a object variable
