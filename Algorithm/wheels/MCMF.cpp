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
#include <utility>
#include <queue>
#include <map>
#include <cstring>
#include <algorithm>
#include <iterator>
#include <string>

using namespace std;
#define REOPEN_READ freopen("/home/martin/X-Brain/Notes/Clang/OnlineJudge/uva/input.txt", "r", stdin);
#define REOPEN_WRITE freopen("/home/martin/X-Brain/Notes/Clang/OnlineJudge/uva/output.txt", "w", stdout);
/**
 * 每一条边除了 容量限制， 还有单位流量的费用
 * 最大流是前提， 
 */

class Edge{
public:
    int x;
    int y;
    int cap;
    int flow;
    int cost;
    Edge(int x, int y, int cap, int flow, int cost): 
    x(x), y(y), cap(cap), flow(flow), cost(cost){};
};

#define maxn 10
#define INF 0x3f3f3f3f

class MCMC{
public:
    int n, m, s, t;
    vector<Edge> edges;
    vector<int> G[maxn];

    // bellman-ford 算法
    bool inq[maxn];
    int d[maxn];
    int p[maxn];
    int a[maxn];

    void init(int n){
        this->n = n;
        for(int i = 0; i < n; i++) G[i].clear();
        edges.clear();
    }

    void addEdge(int x, int y, int cap, int cost){
        edges.push_back(Edge(x, y, cap, 0, cost));
        edges.push_back(Edge(y, x, 0, 0, -cost)); 
        int m = edges.size();
        G[x].push_back(m - 2);
        G[y].push_back(m - 1);
    }
    
        
};

int main(){
    REOPEN_READ
    REOPEN_WRITE

    return 0;
}