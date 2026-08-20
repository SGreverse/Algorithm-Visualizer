#include "pf_algorithms.h"
#include "algo_types.h"
#include "data_structures.h"
#include "pf_types.h"
#include <limits.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>

#define GET_ELEM(ctx,x,y) (ctx->grid[y*ctx->n_columns+x])


//direciton vectors for neigbors
int dr[] = {-1, 1, 0, 0};
int dc[] = {0, 0, -1, 1};

void hook_MarkNode(PFContext* ctx, GridNode* node, NodeState new_state){
    AlgoContext* ctx_b = BASE(ctx);
    
    if (atomic_load(&ctx_b->kill_signal)) return;

    node->state = new_state;
    
    ctx_b->step_counter++;
    if (ctx_b->step_counter < ctx_b->steps_per_frame) {
        return; // Skip the lock and keep running
    }
    
    ctx_b->step_counter = 0;

    mtx_lock(&ctx_b->mutex); 
    ctx_b->frame_consumed = false;
    while(!ctx_b->frame_consumed && !atomic_load(&ctx_b->kill_signal)){
        cnd_wait(&ctx_b->condition_var, &ctx_b->mutex);
    }
    mtx_unlock(&ctx_b->mutex);
}

#pragma region BFS
void BFS_Init(AlgoContext* ctx_b){
    PFContext* ctx=CAST_PF(ctx_b);
    GridNode* curr;
    for(size_t y = 0; y < ctx->n_rows; y++){
        for(size_t x = 0; x < ctx->n_columns; x++){
            GridNode* curr = &GET_ELEM(ctx, x, y);
            curr->gcost = INT_MAX;
            curr->parent_x = -1;
            curr->parent_y = -1;
        }
    }
    curr=&GET_ELEM(ctx,ctx->start_x,ctx->start_y);
    curr->gcost=0;
    curr->state=STATE_FRONTIER;
}
void BFS_Run(AlgoContext* ctx_b){
    PFContext* ctx = CAST_PF(ctx_b);
    Queue* q = createQueue();
    GridNode* current_node = &GET_ELEM(ctx, ctx->start_x, ctx->start_y);
    enqueue(q, current_node);
    
    bool target_found = false;

    while(!isEmpty(q)){
        if(atomic_load(&ctx_b->kill_signal)) break;
        current_node = dequeue(q);
        
        if (current_node->x == ctx->target_x && current_node->y == ctx->target_y) {
            target_found = true;
            break;
        }

        if (!(current_node->x == ctx->start_x && current_node->y == ctx->start_y)) {
            hook_MarkNode(ctx, current_node, STATE_VISITED);
        }

        for (int i = 0; i < 4; i++) {
            int newR = current_node->y + dr[i];
            int newC = current_node->x + dc[i];

            if (newR >= 0 && newR < ctx->n_rows && newC >= 0 && newC < ctx->n_columns) {
                GridNode* neighbor = &GET_ELEM(ctx, newC, newR);
                
                if(neighbor->state == STATE_UNVISITED && !neighbor->is_blocked){
                    
                    hook_MarkNode(ctx, neighbor, STATE_FRONTIER);
                    neighbor->gcost = current_node->gcost + 1;
                    
                    
                    neighbor->parent_x = current_node->x;
                    neighbor->parent_y = current_node->y;
                    
                    enqueue(q, neighbor);
                }
            }
        }
    }
    
    if (target_found) {
        // Start backtracking from the node just before the target
        int trace_x = current_node->parent_x;
        int trace_y = current_node->parent_y;
        
        while (trace_x != -1 && trace_y != -1) {
            if (atomic_load(&ctx_b->kill_signal)) break;
            
            // Stop right before we overwrite the Green start node
            if (trace_x == ctx->start_x && trace_y == ctx->start_y) break;
            
            GridNode* path_node = &GET_ELEM(ctx, trace_x, trace_y);
            hook_MarkNode(ctx, path_node, STATE_PATH);
            
            // Move backward to the next parent
            trace_x = path_node->parent_x;
            trace_y = path_node->parent_y;
        }
    }
    
    free(q);
}
void BFS_Cleanup(AlgoContext* ctx_b){
    //nothing needed
}


const Algorithm BFSAlgorithm = {
    .name = "Breadth-First Search",
    .init = BFS_Init,
    .run = BFS_Run,
    .cleanup = BFS_Cleanup,
    .docs = {
        .overview = "An unweighted pathfinding algorithm that guarantees the shortest path on a uniform grid by exploring all neighbors equally in expanding layers.",
        .process = "1. Add the starting node to a Queue.\n"
                   "2. Dequeue a node and mark it as visited.\n"
                   "3. If the node is the target, reconstruct the path and stop.\n"
                   "4. Otherwise, check all four adjacent neighbors.\n"
                   "5. If a neighbor is valid (unvisited and not a wall), add it to the Queue and record its parent.\n"
                   "6. Repeat until the Queue is empty or the target is found.",
        .time_best = "O(V + E)",
        .time_avg = "O(V + E)",
        .time_worst = "O(V + E)",
        .space_aux_complexity = "O(V) (Queue)",
        .space_recur_complexity = "O(1)"
    }
};
#pragma endregion

#pragma region dijkstra
void dijkstra_Init(AlgoContext* ctx_b){
    PFContext* ctx = CAST_PF(ctx_b);
    
    for(size_t y = 0; y < ctx->n_rows; y++){
        for(size_t x = 0; x < ctx->n_columns; x++){
            GridNode* node = &GET_ELEM(ctx, x, y);
            node->gcost = INT_MAX;
            node->fcost = INT_MAX; //for heap compatability
            node->parent_x = -1;
            node->parent_y = -1;
        }
    }
    
    GridNode* start_node = &GET_ELEM(ctx, ctx->start_x, ctx->start_y);
    start_node->gcost = 0;
    start_node->fcost = 0; 
    start_node->state = STATE_FRONTIER;
}

void dijkstra_Run(AlgoContext* ctx_b){
    PFContext* ctx = CAST_PF(ctx_b);
    Heap* heap=createHeap();
    GridNode* current_node;
    insertHeap(heap, &GET_ELEM(ctx, ctx->start_x, ctx->start_y));

    bool target_found = false;

    while(!isHeapEmpty(heap)){
        current_node=extractMin(heap);
        current_node->state=STATE_VISITED;
        
        if (current_node->x == ctx->target_x && current_node->y == ctx->target_y) {
                target_found = true;
                break;
            }

        if (!(current_node->x == ctx->start_x && current_node->y == ctx->start_y)) {
            hook_MarkNode(ctx, current_node, STATE_VISITED);
        }

        
        for (int i = 0; i < 4; i++) {
            int newR = current_node->y + dr[i];
            int newC = current_node->x + dc[i];

            if (newR >= 0 && newR < ctx->n_rows && newC >= 0 && newC < ctx->n_columns) {
                GridNode* neighbor = &GET_ELEM(ctx, newC, newR);
                int new_cost = current_node->gcost + neighbor->weight;

                if(!neighbor->is_blocked && new_cost < neighbor->gcost){
                    neighbor->gcost = new_cost;
                    
                    
                    neighbor->fcost = new_cost; //to keep node compatible with heap
                    
                    neighbor->parent_x = current_node->x;
                    neighbor->parent_y = current_node->y;
                    
                    if(neighbor->state == STATE_UNVISITED){
                        hook_MarkNode(ctx, neighbor, STATE_FRONTIER);
                        insertHeap(heap, neighbor);
                    }
                    else if (neighbor->state == STATE_FRONTIER) {
                        updatePos(heap, neighbor->heap_index);
                    }
                }
                
            }
        }
    }
    if (target_found) {
        // Start backtracking from the node just before the target
        int trace_x = current_node->parent_x;
        int trace_y = current_node->parent_y;
        
        while (trace_x != -1 && trace_y != -1) {
            if (atomic_load(&ctx_b->kill_signal)) break;
            
            if (trace_x == ctx->start_x && trace_y == ctx->start_y) break;
            
            GridNode* path_node = &GET_ELEM(ctx, trace_x, trace_y);
            hook_MarkNode(ctx, path_node, STATE_PATH);
            
            trace_x = path_node->parent_x;
            trace_y = path_node->parent_y;
        }
    }
    free(heap);

} 
void dijkstra_Cleanup(AlgoContext* ctx_b){
    //nothing needed
}



const Algorithm DijkstraAlgorithm = {
    .name = "Dijkstra's Algorithm",
    .init = dijkstra_Init,
    .run = dijkstra_Run,
    .cleanup = dijkstra_Cleanup,
    .docs = {
        .overview = "A weighted pathfinding algorithm that guarantees the absolute shortest path from a starting node to all other reachable nodes. It forms the foundation of modern network routing and GPS navigation.",
        .process = "1. Assign a distance value of 0 to the start node and infinity to all other nodes.\n"
                   "2. Insert the start node into a Min-Heap (Priority Queue).\n"
                   "3. Extract the node with the lowest recorded distance from the heap and mark it as visited.\n"
                   "4. Calculate the distance to all its unvisited, unblocked neighbors.\n"
                   "5. If the calculated distance is strictly less than the neighbor's currently known distance, update it, record the parent, and insert/update it in the Min-Heap.\n"
                   "6. Repeat until the target is extracted from the heap or the heap becomes empty.",
        .time_best = "O(E log V)",
        .time_avg = "O((V + E) log V)",
        .time_worst = "O((V + E) log V)",
        .space_aux_complexity = "O(V) (Min-Heap)",
        .space_recur_complexity = "O(1)"
    }
};

#pragma endregion

#pragma region Astar
int astar_ManhattenDist(int x1,int y1,int x2,int y2){
    return abs(x1-x2)+abs(y1-y2);
}

void astar_Init(AlgoContext* ctx_b){
    PFContext* ctx = CAST_PF(ctx_b);
    
    for(size_t y = 0; y < ctx->n_rows; y++){
        for(size_t x = 0; x < ctx->n_columns; x++){
            GridNode* node = &GET_ELEM(ctx, x, y);
            node->gcost = INT_MAX;
            node->fcost = INT_MAX; // Add this!
            node->parent_x = -1;
            node->parent_y = -1;
        }
    }
    
    GridNode* start_node = &GET_ELEM(ctx, ctx->start_x, ctx->start_y);
    start_node->gcost = 0;
    
    // Fcost=Hcost because Gcost is 0
    start_node->fcost = astar_ManhattenDist(start_node->x, start_node->y, ctx->target_x, ctx->target_y);
    
    start_node->state = STATE_FRONTIER;
}

void astar_Run(AlgoContext* ctx_b){
    PFContext* ctx = CAST_PF(ctx_b);
    Heap* heap=createHeap();
    GridNode* current_node;
    insertHeap(heap, &GET_ELEM(ctx, ctx->start_x, ctx->start_y));

    bool target_found = false;

    while(!isHeapEmpty(heap)){
        current_node=extractMin(heap);
        current_node->state=STATE_VISITED;
        
        if (current_node->x == ctx->target_x && current_node->y == ctx->target_y) {
                target_found = true;
                break;
            }

        if (!(current_node->x == ctx->start_x && current_node->y == ctx->start_y)) {
            hook_MarkNode(ctx, current_node, STATE_VISITED);
        }

        
        for (int i = 0; i < 4; i++) {
            int newR = current_node->y + dr[i];
            int newC = current_node->x + dc[i];

            if (newR >= 0 && newR < ctx->n_rows && newC >= 0 && newC < ctx->n_columns) {
                GridNode* neighbor = &GET_ELEM(ctx, newC, newR);
                
                int tentative_gcost = current_node->gcost + neighbor->weight;

                if(!neighbor->is_blocked && tentative_gcost < neighbor->gcost){
                    
                    neighbor->gcost = tentative_gcost;
                    
                    int hcost = astar_ManhattenDist(neighbor->x, neighbor->y, ctx->target_x, ctx->target_y);
                    neighbor->fcost = neighbor->gcost + hcost;
                    
                    neighbor->parent_x = current_node->x;
                    neighbor->parent_y = current_node->y;
                    
                    if(neighbor->state == STATE_UNVISITED){
                        hook_MarkNode(ctx, neighbor, STATE_FRONTIER);
                        insertHeap(heap, neighbor);
                    }
                    else if (neighbor->state == STATE_FRONTIER) {
                        updatePos(heap, neighbor->heap_index);
                    }
                }
            }
        }
    }
    if (target_found) {
        int trace_x = current_node->parent_x;
        int trace_y = current_node->parent_y;
        
        while (trace_x != -1 && trace_y != -1) {
            if (atomic_load(&ctx_b->kill_signal)) break;
            
            if (trace_x == ctx->start_x && trace_y == ctx->start_y) break;
            
            GridNode* path_node = &GET_ELEM(ctx, trace_x, trace_y);
            hook_MarkNode(ctx, path_node, STATE_PATH);
            
            trace_x = path_node->parent_x;
            trace_y = path_node->parent_y;
        }
    }
    free(heap);

} 
void astar_Cleanup(AlgoContext* ctx_b){
    //nothing needed
}


const Algorithm AstarAlgorithm = {
    .name = "A* Search",
    .init = astar_Init,
    .run = astar_Run,
    .cleanup = astar_Cleanup, 
    .docs = {
        .overview = "A highly efficient, heuristic-based pathfinding algorithm widely used in game development and AI navigation. It combines the absolute shortest-path guarantee of Dijkstra's Algorithm with the addition of the knowledge of where the goal is to estimate how a specific node is to it.",
        .process = "1. Initialize the start node's G-Cost (distance from start) to 0.\n"
                   "2. Calculate its H-Cost (estimated distance to target using Manhattan distance) and set its F-Cost (G + H).\n"
                   "3. Insert the start node into a Min-Heap sorted by F-Cost.\n"
                   "4. Extract the node with the lowest F-Cost.\n"
                   "5. Check all valid neighbors. If a neighbor offers a shorter G-Cost path, update its G-Cost, recalculate its F-Cost, record the parent, and update the Min-Heap.\n"
                   "6. Repeat until the target node is extracted aaaaaa.",
        .time_best = "O(E)",
        .time_avg = "O(E)", 
        .time_worst = "O(E log V)",
        .space_aux_complexity = "O(V) (Min-Heap)",
        .space_recur_complexity = "O(1)"
    }
};

#pragma endregion