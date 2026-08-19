#include "data_structures.h"
#include "pf_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

//queue
//------------------------------
Queue* createQueue() {
    Queue* q = malloc(sizeof(Queue));
    q->front = -1;
    q->rear = -1;
    return q;
}

bool isEmpty(Queue* q) {
    return q->front == -1;
}

void enqueue(Queue* q, GridNode* node) {
    if (q->rear == MAX_GRID_COLUMNS*MAX_GRID_COLUMNS/2 - 1) {
        return; 
    }
    if (q->front == -1) {
        q->front = 0;
    }
    q->rear++;
    q->items[q->rear] = node;
}

GridNode* dequeue(Queue* q) {
    if (isEmpty(q)) {
        return NULL;
    }
    GridNode* item = q->items[q->front];
    q->front++;
    if (q->front > q->rear) { // Reset queue if it becomes empty
        q->front = q->rear = -1;
    }
    return item;
}

//heap
//-----------------------------------------------
void swap(Heap* heap,size_t i,size_t j)
{
    //update the inner heap index field
    heap->data[i]->heap_index=j;
    heap->data[j]->heap_index=i;
    //swap
    GridNode* temp=heap->data[i];
    heap->data[i]=heap->data[j];
    heap->data[j]=temp;

}
Heap* createHeap(){
    Heap *heap = (Heap *)malloc(sizeof(Heap));
    heap->size = 0;
    return heap;
}
void minHeapify(Heap* heap,size_t i){
    if(i>=heap->size) return;
    size_t smallest = i;
    size_t left = 2 * i + 1;
    size_t right = 2 * i + 2;

    if (left < heap->size && heap->data[left]->fcost < heap->data[smallest]->fcost)
        smallest = left;

    if (right < heap->size && heap->data[right]->fcost < heap->data[smallest]->fcost)
        smallest = right;

    if (smallest != i)
    {
        swap(heap,i,smallest);
        minHeapify(heap, smallest);
    }
}
//after changing weight of a grid node to a smaller weight,update the position using this funciton
void updatePos(Heap* heap, size_t index){
    if (index >= heap->size) return;

    while (index != 0 && heap->data[(index - 1) / 2]->fcost > heap->data[index]->fcost)
    {
        swap(heap,index,(index - 1) / 2);
        index = (index - 1) / 2;
    }
}
void insertHeap(Heap *heap, GridNode* node){
     if (heap->size == MAX_GRID_COLUMNS*MAX_GRID_COLUMNS/2)
    {
        printf("Heap overflow\n");
        return;
    }

    int i = heap->size;
    heap->data[heap->size++]=node;
    node->heap_index=i;

    while (i != 0 && heap->data[(i - 1) / 2]->fcost > heap->data[i]->fcost)
    {
        swap(heap,i,(i - 1) / 2);
        i = (i - 1) / 2;
    }
}
GridNode* extractMin(Heap *heap){
    if (heap->size == 0) return NULL;
    if (heap->size == 1)
    {
        heap->size--;
        return heap->data[0];
    }

    GridNode* root = heap->data[0];
    swap(heap,0,heap->size-1);
    heap->size--;
    minHeapify(heap, 0);

    return root;
}
bool isHeapEmpty(Heap *heap){
    return heap->size==0;
}