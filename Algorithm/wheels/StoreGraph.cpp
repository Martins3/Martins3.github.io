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

int main(){
    
    return 0;
}

# define VERTEX_NUM (100 * 100)
# define EDGE_NUM (100 * 100 * 8)

class Edge{
   public:
       int x;
       int y;
       int head;
       int weight;
       Edge(){};
       Edge(int x, int y, int head):x(x), y(y), head(head){}
       Edge(int x, int y, int head, int weight):x(x), y(y), head(head), weight(weight){}

   };
int nodeHead[VERTEX_NUM];
int nodePointer;
Edge graph[EDGE_NUM];

void addWeightedEdge(int x, int y, int weight){
       graph[nodePointer] = Edge(x, y, nodeHead[x], weight);
       nodeHead[x] = nodePointer;
       nodePointer ++;
   }

void addEdge(int x, int y){
       graph[nodePointer] = Edge(x, y, nodeHead[x]);
       nodeHead[x] = nodePointer;
       nodePointer ++;
   }

void initGraph() {
    memset(nodeHead, -1, sizeof(nodeHead));
    nodePointer = 0;
}