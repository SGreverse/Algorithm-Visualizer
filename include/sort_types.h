#ifndef SORTING_H
#define SORTING_H
#include "documentation.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <threads.h>

typedef struct SortContext SortContext;

    struct SortContext {
        int* array;
        size_t size;
        
        int active_index_a;
        int active_index_b;
        //signal for ui that the sort ended
        atomic_bool is_sorted;
        //safe replcament for thread killing
        atomic_bool kill_signal;
        
        // memory for out-of-place sorts
        void* auxiliary_memory;
        
        //thread-safe comparing two elements and waiting for UI update
        int (*compare)(SortContext* ctx, int idx_a, int idx_b);
        //thread-safe swapping betweeen 2 array elements and waiting for UI update
        void (*swap)(SortContext* ctx, int idx_a, int idx_b);
        //thread-safe writing a value into the array and waiting for UI update
        void (*write)(SortContext* ctx, int idx, int val); 
        
        //lock for thread safe write and read to the array between main thread and worker thread
        mtx_t mutex;
        //blocks worker thread from continuing 
        cnd_t condition_var;
        //ensures UI updates before continuing
        bool frame_consumed;

        //live counters
        size_t compare_count;
        size_t swap_count;
        size_t write_count;
    };

typedef struct{
    char* name;
    AlgoDocs docs;
    void (*init)(SortContext* context);
    void (*sort)(SortContext* context);
    void (*cleanup)(SortContext* context);
}SortAlgorithm;


#endif