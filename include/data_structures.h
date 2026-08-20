

#ifndef DATASTRUCTURES_H
#define DATASTRUCTURES_H

#include "pf_types.h"
#include <stdbool.h>
#include <stddef.h>

//need to make it work with the define in main
#define MAX_GRID_COLUMNS 150
#define GRID_SIZE MAX_GRID_COLUMNS*MAX_GRID_COLUMNS/2
typedef struct{
    GridNode* items[GRID_SIZE];
    int front;
    int rear;
} Queue;

Queue* createQueue();
bool isEmpty(Queue* q);
void enqueue(Queue* q, GridNode* value);
GridNode* dequeue(Queue* q);


//min heap implementation
typedef struct
{
    //init with set max size
    GridNode* data[GRID_SIZE];
    size_t size;
} Heap;

Heap* createHeap();
void minHeapify(Heap* heap,size_t idx);
void updatePos(Heap* heap, size_t index);
void insertHeap(Heap* heap, GridNode* node);
GridNode* extractMin(Heap* heap);
bool isHeapEmpty(Heap* heap);
#endif