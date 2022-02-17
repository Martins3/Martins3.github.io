#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <stack>
#include <sstream>
#include <climits>
#include <forward_list>
#include <deque>
#include <set>
#include <queue>
#include <map>
#include <cstring>
#include <algorithm>
#include <iterator>
#include <string>

using namespace std;
#define REOPEN_READ freopen("/home/martin/X-Brain/Notes/Clang/OnlineJudge/uva/input.txt", "r", stdin);
#define REOPEN_WRITE freopen("/home/martin/X-Brain/Notes/Clang/OnlineJudge/uva/output.txt", "w", stdout);
class Edge{
public:
    int x;
    int y;
    int cap;
    int flow;
    Edge(int x, int y, int cap, int flow): x(x), y(y), cap(cap), flow(flow){};
};
#define maxn 10
#define INF 0x3f3f3f3f
class Dinic{
public:
    int n, m, s, t;
    vector<Edge> edges;
    vector<int> G[maxn];
    bool vis[maxn];
    int d[maxn];

    int cur[maxn]; // 当前弧下标

    /**
     * 如果本身含有两条边反向连接的两个顶点，此函数会导致四条边的出现，
     * 重边影响必定含有
     */
    void addEdge(int start, int end, int cap){
        edges.push_back(Edge(start, end, cap, 0));
        edges.push_back(Edge(end, start, cap, 0));
        int m = edges.size();
        G[start].push_back(m - 2);
        G[end].push_back(m - 1);
    }

    bool BFS(){
        memset(vis, 0, sizeof(vis));

        queue<int> Q;
        Q.push(s);
        d[s] = 0;
        vis[s] = true;

        // 没有使用 层级的遍历方法，　而是直接就可以添加数值，　确定最短距离数值
        while(!Q.empty()){
            int x = Q.front(); Q.pop();

            for(int i = 0; i < G[x].size(); i++){
                Edge & e = edges[G[x][i]];

                if(!vis[e.y] && e.cap > e.flow){
                    vis[e.x] = true;
                    d[e.y] = d[x] + 1;
                    Q.push(e.y);
                }
            }
        }
        return vis[t];
    }

    /**
     * x 当前的节点编号，a　当前的容量
     * 当查询a  = 0 的时候，　直接放弃, 相当于的一个节点的走到死路中间的
     * 在遍历之前，　首先的处理的是文件， x 和 a 表示到达该节点还有的flow
     * 在残差图中间， cap 总是不变的， flow 维持为整个图flow ,而不是一次查询的flow
     * flow 数值可以成为负数， 但是只要容量大于 flow 都是表示可以通过的
     * 为什么可以出现的多次的dfs, 一次不就是可以解决了吗 ？
     *
     * 直接使用bfs 实现最短路径查询， 是否可以 ？
     * 说明处理结束之后， 最短路发生变动， 以及dfs 没有处理完成
     */
    int dfs(int x, int a){
        if(x == t || a == 0) return a;

        int flow = 0, f;
        // f 用于 一条路径的上面的查询， flow 用于对于多条路径的查询， 只要可以实现
        // cur 用于同一个循环中间的dfs

        // 使用　ref i ，　cur 开始位置是０， 但是之后的 dfs 开始的就是不会如此
        for(int & i = cur[x]; i < G[x].size(); i++){
            Edge & e = edges[G[x][i]];

            //  使用dfs  查询容量
            if(d[x] + 1 == d[e.y] && (f = dfs(e.y, min(a, e.cap - e.flow))) > 0){

                // 一旦成功， 立刻修改flow，
                e.flow += f;
                edges[G[x][i]^1].flow -= f;

                // 实际容量 和 耗尽的容量， 为什么可以实现总是
                flow += f;
                a -= f;
                if(a == 0) break;
            }
        }
        return flow;
    }

    int maxFlow(int s, int t){
        this->s = s; this->t = t;
        int flow = 0;
        while(BFS()){
            memset(cur, 0, sizeof(cur));
            flow += dfs(s, INF);
        }
        return flow;
    }
};


int main(){
    REOPEN_READ
    REOPEN_WRITE

    return 0;
}
