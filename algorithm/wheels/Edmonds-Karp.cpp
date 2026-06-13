#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <stack>
#include <sstream>
#include <climits>
#include <forward_list>
#include <deque>
#include <queue>
#include <map>
#include <cstring>
#include <algorithm>
#include <iterator>
#include <string>

using namespace std;
#define REOPEN_READ freopen("/home/martin/X-Brain/Notes/Clang/OnlineJudge/uva/input.txt", "r", stdin);
#define REOPEN_WRITE freopen("../output.txt", "w", stdout);

/**
 * 最小费用流
 *  1. 查找增加广度路径的时候， 使用 加速的bellman-ford 算法来处理
 *  2.
 */
class Edge{
public:
    int from, to, cap, flow;
    Edge(int from, int to, int cap, int flow): from(from), to(to), cap(cap), flow(flow){}
};

#define maxn 10
#define INF 0x3f3f3f3f
class EdmondsKarp{
public:
    int n; // 顶点的个数
    int m; // 边的位置序号
    vector<Edge> edges;
    vector<int> G[maxn];
    int a[maxn]; // 从起点到达i 可以使用的流量， 用于代替的 vis数组
    int p[maxn]; // 最短弧 p 的入编号, 用于实现记录路径的

    void init(){
        for(int i = 0; i < n; i++) G[i].clear();
        edges.clear();
    }

    void add_adge(int from, int to, int cap){
        edges.push_back(Edge(from, to, cap, 0));
        edges.push_back(Edge(to, from, cap, 0));
        m = edges.size();
        G[from].push_back(m - 2);
        G[to].push_back(m - 1);
    }

    int max_flow(int s, int t){
        int flow = 0;
        while(true){
            memset(a, 0, sizeof(a));
            queue<int> Q;
            Q.push(s);
            a[s] = INF;

            while(!Q.empty()){
                int x = Q.front();
                Q.pop();

                // bfs
                for(int i = 0; i < G[x].size(); i++){
                    Edge & e = edges[G[x][i]];

                    if(!a[e.to] && e.cap > e.flow){
                        p[e.to] = G[x][i];
                        a[e.to] = min(a[x], e.cap - e.flow);
                        Q.push(e.to);
                    }
                }
                if(a[t]) break;
            }
            if(!a[t]) break;

            // 处理结果
            for(int i = t; i != s; i = edges[p[i]].from){
                edges[p[i]].flow += a[t];
                edges[p[i] ^ 1].flow -= a[t];
            }
            flow += a[t];
        }
        return flow;
    }

};
int main(){
    return 0;
}
