#ifndef ALGOTYPES_H
#define ALGOTYPES_H

#include "documentation.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <threads.h>

//casts the context to its base struct
#define BASE(ctx) ((AlgoContext*)ctx)

typedef struct{
    atomic_bool kill_signal;
    //lock for thread safe write and read to the array between main thread and worker thread
    mtx_t mutex;
    //blocks worker thread from continuing 
    cnd_t condition_var;
    //ensures UI updates before continuing
    bool frame_consumed;

    atomic_bool is_finished;

    //for going past the ralylib refresh rate limitation by batch executing
    //optional for each algorithm
    int step_counter;
    int steps_per_frame;
}AlgoContext;

typedef struct{
    char* name;
    AlgoDocs docs;
    void (*init)(AlgoContext* context);
    void (*run)(AlgoContext* context);
    void (*cleanup)(AlgoContext* context);
}Algorithm;

#endif