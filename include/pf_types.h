#ifndef PATHFINDING_H
#define PATHFINDING_H
#include "algo_types.h"

#define CAST_PF(ctx_b) ((PFContext*)ctx_b)


typedef enum {
    STATE_UNVISITED,
    STATE_FRONTIER,  // The 'Open Set' (The algorithm is considering checking this)
    STATE_VISITED,   // The 'Closed Set' (The algorithm has fully explored this)
    STATE_PATH       // The final reconstructed route
} NodeState;

typedef struct{
    bool is_blocked;   
    bool is_heavier;
    int weight;

    int gcost;          // movement cost
    int fcost;
    
    NodeState state;     //determines the color
    
    //for path recostruction
    int parent_x;
    int parent_y;

    size_t x;
    size_t y;

    //tracks nodes in the  heap(inits to -1 if not in)
    size_t heap_index;

}GridNode;

typedef struct PFContext PFContext;

struct PFContext{
    AlgoContext base;

    //1d array mapping the 2d grid, to increase cache hits
    GridNode* grid;
    size_t n_rows;
    size_t n_columns;

    size_t start_x;
    size_t start_y;
    size_t target_x;
    size_t target_y;



    void (*mark_node)(PFContext* ctx, GridNode* node, NodeState new_state);

};

#endif