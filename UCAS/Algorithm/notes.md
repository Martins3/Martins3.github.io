# 1, 3, 4, 7, 14 章

复杂性:
| -     | desc |
|-------|------|
| P     | 求解 |
| NP    | 验证 |
| co-NP |
| RP    |
| co-RP |
| BPP   |
| ZPP   |


complexity : a set of language
language : a set of input 


1. Matrix Multiplication Verification
    1. 为什么需要保证在素数 p 的同余类上
    2. p 是什么东西 ?
2. NP 定义的辅助证据 w 是什么 ?


https://www.cs.cmu.edu/~avrim/Randalgs11/lectures/lect0209.pdf


复杂类补充资料 :
1. The travelling salesman problem (also called the travelling salesperson problem[1] or TSP) asks the following question: 
"Given a list of cities and the distances between each pair of cities, what is the shortest possible route that visits each city and returns to the origin city?"
It is an NP-hard problem in combinatorial optimization, important in operations research and theoretical computer science.
> 经过所有的节点最短路程

2. In the mathematical field of graph theory the Hamiltonian path problem and the Hamiltonian cycle problem are problems of determining whether a **Hamiltonian path** (a path in an undirected or directed graph that visits each vertex exactly once) or a **Hamiltonian cycle** exists in a given graph (whether directed or undirected). Both problems are NP-complete.[1]

The Hamiltonian cycle problem is a special case of the travelling salesman problem, obtained by setting the distance between two cities to one if they are adjacent and two otherwise, and verifying that the total distance travelled is equal to n (if so, the route is a Hamiltonian circuit; if there is no Hamiltonian circuit then the shortest route will be longer).
> 当 TSP 中间所有的路径长度都是 1, 如果 TSP 的结果正好就是 n

3. https://en.wikipedia.org/wiki/Matching_(graph_theory)

4. https://math.stackexchange.com/questions/86210/what-is-the-3-sat-problem


**一个随机算法 A 除了接收输入串 x 外，还可以使用一些随机比特 r 来计算输出结果 A(x, r)。这
些随机比特的引入可能使得算法的结果出错（RP、 co-RP、 BPP），也可能使得算法的运行时间变成一
个随机变量（ZPP）。**
