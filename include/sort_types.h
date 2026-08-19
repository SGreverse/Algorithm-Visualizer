#ifndef SORTING_H
#define SORTING_H
#include "algo_types.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <threads.h>

//casts base algorithm context to sort context
#define CAST_SORT(ctx_b) ((SortContext*)ctx_b)

typedef struct SortContext SortContext;

struct SortContext {
    AlgoContext base;
    int* array;
    size_t size;
        
    int active_index_a;
    int active_index_b;

        
    // memory for out-of-place sorts
    void* auxiliary_memory;
        
    //thread-safe comparing two elements and waiting for UI update
    int (*compare)(SortContext* ctx, int idx_a, int idx_b);
    //thread-safe swapping betweeen 2 array elements and waiting for UI update
    void (*swap)(SortContext* ctx, int idx_a, int idx_b);
    //thread-safe writing a value into the array and waiting for UI update
    void (*write)(SortContext* ctx, int idx, int val); 
    //live counters
    size_t compare_count;
    size_t swap_count;
    size_t write_count;
};


#endif