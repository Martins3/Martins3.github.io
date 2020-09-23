#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <stack>
#include <queue>
#include <map>
#include <cstring>
#include <algorithm>

using namespace std;
#define REOPEN_READ freopen("../input.txt", "r", stdin);
#define REOPEN_WRITE freopen("../output.txt", "w", stdout);

/**
 * 如果有一个大小为n的 clique， 那么原来图形上面一定有一个大小为 n 的SSC
 * 相同的也是如此。
 * 也就是只需要寻找到最大的SSC就是可以了
 */

# define VERTEX_NUM 1010
# define EDGE_NUM 50000

class Edge{
   public:
        int x;
       int y;
       int head;
       Edge(){};
       Edge(int x, int y, int head):  x(x), y(y), head(head){}
};

int nodeHead[VERTEX_NUM];
int nodePointer;
Edge graph[EDGE_NUM];

int anti_nodeHead[VERTEX_NUM];
int anti_nodePointer;
Edge anti_graph[EDGE_NUM];

void addEdge(int x, int y){
    graph[nodePointer] = Edge(x, y, nodeHead[x]);
    nodeHead[x] = nodePointer;
    nodePointer ++;
}

void anti_addEdge(int x, int y){
    anti_graph[anti_nodePointer] = Edge(x, y, anti_nodeHead[x]);
    anti_nodeHead[x] = anti_nodePointer;
    anti_nodePointer ++;
}

void initGraph() {
    memset(nodeHead, -1, sizeof(nodeHead));
    nodePointer = 0;
    memset(anti_nodeHead, -1, sizeof(anti_nodeHead));
    anti_nodePointer = 0;
}

int vertex_num;
int edge_num;

void readGraph(){
    initGraph();
    scanf("%d %d", &vertex_num, &edge_num);
    for(int i = 0; i < edge_num; i++){
        int x, y;
        scanf("%d %d", &x, &y);
        addEdge(x, y);
    }
}


/**
 *
 * 对于原来的图形dfs 数据保存在的一个stack 中间
 * 然后实现出 stack 实现每一个点的遍历
 *
 * 如果 a b, 那么 a 和 b 的关系是没有 从 b 到达 a 和  有， 但是绝对不会是只有 a 到 b， 此时采用反项图，
 * 如果b 可以到达 a, 那么a 必定可以到b
 */
bool vis[1010];
int res;
int affection_num;
stack<int> s;

void dfs_one(int x){
    vis[x] = true;
    for(int i = nodeHead[x]; i != -1; i = graph[i].head){
        int y = graph[i].y;
        if(!vis[y]){
            dfs_one(y);
        }
    }
    s.push(x);
}


void dfs_two(int x){
    affection_num ++;
    vis[x] = true;
    for(int i = anti_nodeHead[x]; i != -1; i = anti_graph[i].head){
        int y = anti_graph[i].y;
        if(!vis[y]){
            dfs_two(y);
        }
    }
}


void generate_anti_graph(){
    for(int i = 0; i < nodePointer; i++){
        int x = graph[i].x;
        int y = graph[i].y;
        anti_addEdge(y, x);
    }
}


void SCC(){
    // 对于原来的图 dfs
    memset(vis, false, sizeof(vis));
    for(int i = 1; i <= vertex_num; i++){
        if(!vis[i]) dfs_one(i);
    }

    generate_anti_graph();
    // 对于所有的点dfs
    memset(vis, false, sizeof(vis));
    while(!s.empty()){
        int x = s.top();
        s.pop();
        cout << x << " aaa" << endl;
        if(vis[x]) continue;
        affection_num = 0;
        dfs_two(x);
        // cout << x << " " <<  affection_num << " fff" << endl;
        res = max(res, affection_num);
    }
    printf("%d\n", res);
}


int main(){
    // n = 0 m = 0
    // 从 1 开始的
    // REOPEN_WRITE
    REOPEN_READ
    int N;
    scanf("%d", &N);
    while(N --){
        readGraph();
        SCC();
    }
    return 0;
}
