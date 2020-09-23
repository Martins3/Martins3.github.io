#ifndef BIN_HEAP_H
#define BIN_HEAP_H

struct HeapStruct;
typedef struct HeapStruct * PriorityQueue;

PriorityQueue * Initialize(int maxSize);
void destroy(PriorityQueue p);
void makeEmpty(PriorityQueue p);
void insert(int x, PriorityQueue p);
int deleteMin(PriorityQueue p);
int min(PriorityQueue p);
int isEmpty(PriorityQueue p);
int isFull(PriorityQueue p);
#endif
