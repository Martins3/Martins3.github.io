# dfs
visted数组 -- 刚刚进入的时候添加标记


## recursive to loop
应该不会存在这么变态的需求吧

## 查找 cut vertex 和 bridge

算法: 节点维持两个变量a, b。 a 永远不变， b 结束更新
初次遇见的时候，a = b = count ++
b的在结束的修改， 取出所有的最小值。

cut vertex : 存在一个 out edge， b小于等于。
cut edge ： 两个端点 b 差值为1

cut vertex 特殊处理， cut edge不需要特殊处理。
根节点需要特殊考察分析， 对于一个环，在以上的判断中间会失效， 更节点一旦含有超过
连个出边是了。

## Euler Circuits
1. 所有点度数是偶数， 构成回路
2. 两个点的度数是偶数， 构成路径
3. 有方向图:
    1. 所有点上的出度和入度全部相同
    2. 如果不同，需要成组对应
算法: Hierholzer's algorithm 使用 doubly linked list

## Topologic sort
topo排序的前提 ： 如果含有A -> B的边， 那么就一定B必定首先放置到A的前面
当一个点结束访问时候放入stack中间。

## SCC(Strongly Connected Component)
对于所有点进行拓扑排序操作， 得到一个标记顺序。 a -> b。
构建反图
按照结束的顺序进行dfs, 每次得到的都是一个SCC, 首先访问 a

证明 : v w在同一个， 那么必定通过该点的算法，可以归到一个CC中间
如果v w 不在， 那么不可能在一起。


# 广度优先
visted数组
queue ： 点被加入到queue的同时立刻添加标记

# 最小生成树

## Prim
类似于 Dijkstra，删除distance数组

## Kruskal
使用Heap和union-find实现

# 最短路算法

## Dijkstra
distance 数组
visited数组 -- 除了开始的点， 其余的出来的时候被标记
Heap 中间存放的是边

添加进入的条件:
1. y访问过
2. 没有办法减少距离

出来的边被使用的条件：
没有被标记过。

## Bellman-Ford
算法：所有的边 来 依次更新 距离， 更新次数为 size(vertex) - 1
动态规划： 每一步计算都是保证第i步可以到达点将会在文件中间。

## Floyed-Warshall
注意矩阵的对称和中心的位置。
for k for i for j if dis[i][k] +  dis[k][j] < dis[i][j]

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


