#include "algo_types.h"
#include "pf_algorithms.h"
#include "pf_types.h"
#include "raylib.h"
#include "sort_algorithms.h"
#include "sort_types.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>
#include <stdio.h> 

#define MAX_VAL 400
#define SCREEN_WIDTH 1000  // Widened slightly for better taWb fit
#define SCREEN_HEIGHT 700
#define UI_HEIGHT 170

#define MAX_ARRAY_SIZE 1000
#define MIN_ARRAY_SIZE 10

#define MIN_GRID_COLS 10
#define MAX_GRID_COLS 150

#define MIN_SPEED 1
#define MAX_SPEED 1000

#define IS_WEIGHTED_ALGO() active_algo!=&BFSAlgorithm
typedef enum {
    MODE_SORTING,
    MODE_PATHFINDING
} AppMode;

const char* APP_MODES[] = { "SORTING", "PATHFINDING" };
const int NUM_MODES = 2;

// --- ALGORITHM ARRAYS ---
const Algorithm* SORTING_ALGORITHMS[] = {
    &BubbleSortAlgo,
    &InsertionSortAlgo,
    &SelectionSortAlgo,
    &QuickSortAlgo,
    &MergeSortAlgo,
    &CountingSortAlgo,
    &RadixSortAlgo,
    &HeapSortAlgo
};

// Mock Pathfinding algorithms so the code compiles and runs

const Algorithm* PATHFINDING_ALGORITHMS[] = {
    &BFSAlgorithm,
    &DijkstraAlgorithm,
    &AstarAlgorithm
};

const Algorithm* active_algo = &BubbleSortAlgo; 

int workerThread_Run(void* arg) {
    AlgoContext* ctx = (AlgoContext*)arg;
    active_algo->init(ctx);
    active_algo->run(ctx);
    
    // Only flag as finished if not killed mid-run
    if (!atomic_load(&ctx->kill_signal)) {
        atomic_store(&ctx->is_finished, true);
    }
    active_algo->cleanup(ctx);
    return 0;
}

void randomizeArray(SortContext* ctx) {
    for (size_t i = 0; i < ctx->size; i++) {
        ctx->array[i] = (rand() % MAX_VAL) + 10; 
    }
    atomic_store(&BASE(ctx)->is_finished, false);
    
    // Reset the counters
    ctx->compare_count = 0;
    ctx->swap_count = 0;
    ctx->write_count = 0;
}

void randomizeGrid(PFContext* ctx,bool is_weighted) {
    atomic_store(&ctx->base.is_finished, false);
    
    for (size_t y = 0; y < ctx->n_rows; y++) {
        for (size_t x = 0; x < ctx->n_columns; x++) {
            GridNode* node = &ctx->grid[y * ctx->n_columns + x];
            node->state = STATE_UNVISITED;
            
            node->is_blocked = ((rand() % 100) < 25); 
            if(is_weighted && !node->is_blocked && ((rand() % 100) < 15)){
                //15% chance for it to be a slower cell
                node->weight=5;
                node->is_heavier=true;
            }
            else{
                node->weight=1;
                node->is_heavier=false;
            }
            node->x=x;
            node->y=y;
        }
    }
    
    ctx->grid[ctx->start_y * ctx->n_columns + ctx->start_x].is_blocked = false;
    ctx->grid[ctx->target_y * ctx->n_columns + ctx->target_x].is_blocked = false;
}

void stopAlgorithm(AlgoContext* ctx, thrd_t* worker_thread) {
    atomic_store(&ctx->kill_signal, true);
    
    mtx_lock(&ctx->mutex);
    ctx->frame_consumed = true; 
    cnd_broadcast(&ctx->condition_var);
    mtx_unlock(&ctx->mutex);

    thrd_join(*worker_thread, NULL); 
}

void startAlgorithm(AlgoContext* ctx, thrd_t* worker_thread) {
    atomic_store(&ctx->kill_signal, false);
    atomic_store(&ctx->is_finished, false);
    thrd_create(worker_thread, workerThread_Run, ctx);
}

void drawTextWrapped(Font font, const char* text, Vector2 position, float fontSize, float spacing, float maxWidth, Color tint) {
    float currentY = position.y;
    float currentX = position.x;
    
    // Split text by words and newlines
    const char* ptr = text;
    const char* wordStart = text;
    
    while (*ptr != '\0') {
        if (*ptr == ' ' || *ptr == '\n' || *(ptr + 1) == '\0') {
            int length = (ptr - wordStart) + (*(ptr + 1) == '\0' ? 1 : 0);
            
            // Extract the word
            char word[256] = {0}; 
            for(int i = 0; i < length && i < 255; i++) word[i] = wordStart[i];
            
            Vector2 wordSize = MeasureTextEx(font, word, fontSize, spacing);
            
            // Check if we need to wrap to the next line
            if (currentX + wordSize.x > position.x + maxWidth) {
                currentX = position.x;
                currentY += fontSize + 5; // Line height
            }
            
            DrawTextEx(font, word, (Vector2){currentX, currentY}, fontSize, spacing, tint);
            
            // Advance positions
            if (*ptr == '\n') {
                currentX = position.x;
                currentY += fontSize + 5;
            } else {
                currentX += wordSize.x + MeasureTextEx(font, " ", fontSize, spacing).x;
            }
            
            wordStart = ptr + 1;
        }
        ptr++;
    }
}
int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Algorithm Visualizer");
    SetTargetFPS(60);
    srand(time(NULL));
    
    //sorting Context
    int data_array[MAX_ARRAY_SIZE];
    SortContext sort_ctx = {0};
    sort_ctx.array = data_array;
    sort_ctx.size = 150; 
    sort_ctx.compare=hook_Compare;
    sort_ctx.swap=hook_Swap;
    sort_ctx.write=hook_Write;
    atomic_init(&sort_ctx.base.is_finished, false);
    atomic_init(&sort_ctx.base.kill_signal, false);
    mtx_init(&sort_ctx.base.mutex, mtx_plain);
    cnd_init(&sort_ctx.base.condition_var);
    randomizeArray(&sort_ctx);

    // pathfinding Context
    PFContext pf_ctx = {0};
    pf_ctx.n_columns = 40;
    pf_ctx.n_rows = 20;
    pf_ctx.start_x = 2;
    pf_ctx.start_y = 2;
    pf_ctx.target_x = pf_ctx.n_columns - 3;
    pf_ctx.target_y = pf_ctx.n_rows -3;
    
    pf_ctx.grid = calloc(pf_ctx.n_columns * pf_ctx.n_rows, sizeof(GridNode)); 
    atomic_init(&pf_ctx.base.is_finished, false);
    atomic_init(&pf_ctx.base.kill_signal, false);
    mtx_init(&pf_ctx.base.mutex, mtx_plain);
    cnd_init(&pf_ctx.base.condition_var);
    pf_ctx.mark_node = hook_MarkNode;
    randomizeGrid(&pf_ctx,false);

    // --- STATE VARIABLES ---
    AppMode current_mode = MODE_SORTING;
    AlgoContext* active_context = (AlgoContext*)&sort_ctx;

    thrd_t worker_thread;
    int target_speed = 60; 
    bool is_dragging_speed = false;
    float step_timer = 0.0f;
    
    bool is_running = false;
    bool is_dragging_slider = false;
    bool is_docs_shown = false;
    bool is_sweeping = false;
    float sweep_timer = 0.0f;
    int sweep_index = -1;
    const float SWEEP_DURATION = 1.5f;

    while (!WindowShouldClose()) { 
        Vector2 mouse = GetMousePosition();

        if (is_running && atomic_load(&active_context->is_finished)) {
            thrd_join(worker_thread, NULL);
            is_running = false;
            
            if (current_mode == MODE_SORTING) {
                is_sweeping = true;
                sweep_timer = 0.0f;
                sweep_index = 0;
            }
        }

        if (is_sweeping && current_mode == MODE_SORTING) {
            sweep_timer += GetFrameTime();
            sweep_index = (int)((sweep_timer / SWEEP_DURATION) * sort_ctx.size);
            
            if (sweep_index >= (int)sort_ctx.size) {
                is_sweeping = false;
                sweep_index = sort_ctx.size;
            } 
        } 

        BeginDrawing();
        ClearBackground(GetColor(0x121212FF)); 

        //UI
        DrawRectangle(0, 0, SCREEN_WIDTH, UI_HEIGHT, GetColor(0x1F2937FF)); 
        DrawLine(0, UI_HEIGHT, SCREEN_WIDTH, UI_HEIGHT, GetColor(0x374151FF)); 

        // mode navbar
        float mode_tab_width = (float)SCREEN_WIDTH / NUM_MODES;
        for (int i = 0; i < NUM_MODES; i++) {
            Rectangle tab = { i * mode_tab_width, 0, mode_tab_width, 30 };
            bool is_hover = CheckCollisionPointRec(mouse, tab);
            bool is_active = (current_mode == (AppMode)i);

            if (is_active) DrawRectangleRec(tab, GetColor(0x2563EBFF)); // Darker Blue
            else if (is_hover) DrawRectangleRec(tab, GetColor(0x374151FF)); 
            else DrawRectangleRec(tab, GetColor(0x111827FF)); // Very Dark Gray

            DrawRectangleLinesEx(tab, 1, BLACK);
            int text_width = MeasureText(APP_MODES[i], 12);
            DrawText(APP_MODES[i], tab.x + (mode_tab_width / 2) - (text_width / 2), tab.y + 10, 12, is_active ? WHITE : LIGHTGRAY);

            // Mode Switching Logic
            if (!is_docs_shown && is_hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !is_running && !is_active) {
                current_mode = (AppMode)i;
                is_sweeping = false; // Kill animations
                
                // Route the pointers to the correct system
                if (current_mode == MODE_SORTING) {
                    active_algo = SORTING_ALGORITHMS[0];
                    active_context = BASE(&sort_ctx);
                } else if (current_mode == MODE_PATHFINDING) {
                    active_algo = PATHFINDING_ALGORITHMS[0];
                    active_context = BASE(&pf_ctx);
                }
            }
        }

        // algorithm navbar
        const Algorithm** current_algos_list = (current_mode == MODE_SORTING) ? SORTING_ALGORITHMS : PATHFINDING_ALGORITHMS;
        int num_algos = (current_mode == MODE_SORTING) ? (sizeof(SORTING_ALGORITHMS)/sizeof(SORTING_ALGORITHMS[0])) : (sizeof(PATHFINDING_ALGORITHMS)/sizeof(PATHFINDING_ALGORITHMS[0]));
        
        float algo_tab_width = (float)SCREEN_WIDTH / num_algos;
        for (int i = 0; i < num_algos; i++) {
            Rectangle tab = { i * algo_tab_width, 30, algo_tab_width, 40 }; // Y is 30
            bool is_hover = CheckCollisionPointRec(mouse, tab);
            bool is_active = (active_algo == current_algos_list[i]);

            if (is_active) DrawRectangleRec(tab, GetColor(0x3B82F6FF)); 
            else if (is_hover) DrawRectangleRec(tab, GetColor(0x4B5563FF)); 
            else DrawRectangleRec(tab, GetColor(0x1F2937FF)); 

            DrawRectangleLinesEx(tab, 1, GetColor(0x111827FF));
            int text_width = MeasureText(current_algos_list[i]->name, 10);
            DrawText(current_algos_list[i]->name, tab.x + (algo_tab_width / 2) - (text_width / 2), tab.y + 15, 10, WHITE);

            if (!is_docs_shown && is_hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !is_running) {
                active_algo = current_algos_list[i];
                if (current_mode == MODE_SORTING) randomizeArray(&sort_ctx);
                else{
                    randomizeGrid(&pf_ctx,IS_WEIGHTED_ALGO());
                }
            }
        }

        // main control buttons
        Rectangle btn = { 30, 90, 160, 50 }; // Shifted down
        bool btn_hover = CheckCollisionPointRec(mouse, btn);
        
        if (!is_running && !atomic_load(&active_context->is_finished)) {
            DrawRectangleRec(btn, btn_hover ? GetColor(0x10B981FF) : GetColor(0x059669FF)); 
            DrawText("START", btn.x + 45, btn.y + 15, 20, WHITE);
            if (btn_hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                startAlgorithm(active_context, &worker_thread);
                is_running = true;
            }
        } 
        else if (is_running) {
            DrawRectangleRec(btn, btn_hover ? GetColor(0xEF4444FF) : GetColor(0xDC2626FF)); 
            DrawText("STOP", btn.x + 55, btn.y + 15, 20, WHITE);
            if (btn_hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                stopAlgorithm(active_context, &worker_thread);
                is_running = false;
            }
        }
        else if (atomic_load(&active_context->is_finished)) {
            DrawRectangleRec(btn, btn_hover ? GetColor(0x8B5CF6FF) : GetColor(0x7C3AEDFF)); 
            DrawText("RESET", btn.x + 45, btn.y + 15, 20, WHITE);
            if (btn_hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (current_mode == MODE_SORTING) {
                    randomizeArray(&sort_ctx);
                } else if (current_mode == MODE_PATHFINDING) {
                    // Reset the grid!
                    randomizeGrid(&pf_ctx,IS_WEIGHTED_ALGO());
                }
            }
        }
        
        Rectangle info_btn = { 220, 90, 160, 50 };
        bool info_btn_hover = CheckCollisionPointRec(mouse, info_btn);
        DrawRectangleRec(info_btn, BLACK); 
        DrawText("DOCS", info_btn.x + 45, info_btn.y + 15, 20, WHITE);
        if (info_btn_hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            is_docs_shown = true;
        }

        // --- DYNAMIC CONTROLS (Only show relevant sliders) ---
        if (current_mode == MODE_SORTING) {
            // Array Size Slider
            Rectangle track = { 420, 100, 260, 6 };
            DrawRectangleRec(track, GetColor(0x4B5563FF));
            
            float percent = (float)(sort_ctx.size - MIN_ARRAY_SIZE) / (MAX_ARRAY_SIZE - MIN_ARRAY_SIZE);
            Rectangle knob = { track.x + (percent * track.width) - 8, track.y - 7, 16, 20 };
            
            char sizeText[32];
            sprintf(sizeText, "Array Size: %zu", sort_ctx.size);
            DrawText(sizeText, track.x, track.y - 25, 20, LIGHTGRAY);

            if (!is_running) {
                DrawRectangleRec(knob, is_dragging_slider ? WHITE : LIGHTGRAY);
                Rectangle hitBox = { track.x - 20, track.y - 20, track.width + 40, track.height + 40 };
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && (CheckCollisionPointRec(mouse, hitBox) || is_dragging_slider)) {
                    is_dragging_slider = true;
                    float normalized = (mouse.x - track.x) / track.width;
                    if (normalized < 0.0f) normalized = 0.0f;
                    if (normalized > 1.0f) normalized = 1.0f;
                    size_t calculated_size = MIN_ARRAY_SIZE + (size_t)(normalized * (MAX_ARRAY_SIZE - MIN_ARRAY_SIZE));
                    
                    if (calculated_size != sort_ctx.size) {
                        sort_ctx.size = calculated_size;
                        randomizeArray(&sort_ctx); 
                    }
                }
            }
            else{
                    DrawRectangleRec(knob, GetColor(0x374151FF)); // Disabled during run
                }
        }
        else if (current_mode == MODE_PATHFINDING) {
            // --- Pathfinding Grid Size Slider ---
            Rectangle track = { 420, 100, 260, 6 };
            DrawRectangleRec(track, GetColor(0x4B5563FF));
            
            float percent = (float)(pf_ctx.n_columns - MIN_GRID_COLS) / (MAX_GRID_COLS - MIN_GRID_COLS);
            Rectangle knob = { track.x + (percent * track.width) - 8, track.y - 7, 16, 20 };
            
            char sizeText[32];
            sprintf(sizeText, "Grid Size: %d x %d", pf_ctx.n_columns, pf_ctx.n_rows);
            DrawText(sizeText, track.x, track.y - 25, 20, LIGHTGRAY);

            if (!is_running) {
                DrawRectangleRec(knob, is_dragging_slider ? WHITE : LIGHTGRAY);
                Rectangle hitBox = { track.x - 20, track.y - 20, track.width + 40, track.height + 40 };
                
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && (CheckCollisionPointRec(mouse, hitBox) || is_dragging_slider)) {
                    is_dragging_slider = true;
                    float normalized = (mouse.x - track.x) / track.width;
                    if (normalized < 0.0f) normalized = 0.0f;
                    if (normalized > 1.0f) normalized = 1.0f;
                    
                    size_t calc_cols = MIN_GRID_COLS + (size_t)(normalized * (MAX_GRID_COLS - MIN_GRID_COLS));
                    
                    // If the column size actually changed, we must reallocate memory
                    if (calc_cols != pf_ctx.n_columns) {
                        pf_ctx.n_columns = calc_cols;
                        pf_ctx.n_rows = calc_cols / 2; // Maintain 2:1 visual aspect ratio
                        
                        // keep Start and Target safely in bounds
                        pf_ctx.start_x = 2;
                        pf_ctx.start_y = pf_ctx.n_rows / 2;
                        pf_ctx.target_x = pf_ctx.n_columns - 3;
                        pf_ctx.target_y = pf_ctx.n_rows / 2;

                        // memory reallocation
                        free(pf_ctx.grid);
                        pf_ctx.grid = calloc(pf_ctx.n_columns * pf_ctx.n_rows, sizeof(GridNode));
                        
                        randomizeGrid(&pf_ctx,IS_WEIGHTED_ALGO());
                    }
                }
            } else {
                DrawRectangleRec(knob, GetColor(0x374151FF)); // Disabled during run
            }
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) is_dragging_slider = false;
        }
        


        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) is_dragging_slider = false;


        // Sorting Stats
        if(current_mode==MODE_SORTING){
            int stats_x = SCREEN_WIDTH - 280;
            int stats_y = 85;
            DrawText(TextFormat("Compares: %zu", sort_ctx.compare_count), stats_x, stats_y, 16, GetColor(0x9CA3AFFF));
            DrawText(TextFormat("Swaps: %zu", sort_ctx.swap_count), stats_x, stats_y + 20, 16, GetColor(0x9CA3AFFF)); 
            DrawText(TextFormat("Writes: %zu", sort_ctx.write_count), stats_x, stats_y + 40, 16, GetColor(0x9CA3AFFF));
        }
        // Speed slider logic (Applies to ALL algorithms)
        Rectangle speed_track = { 420, 145, 260, 6 };
        DrawRectangleRec(speed_track, GetColor(0x4B5563FF));
        float speed_percent = (float)(target_speed - MIN_SPEED) / (MAX_SPEED - MIN_SPEED);
        Rectangle speed_knob = { speed_track.x + (speed_percent * speed_track.width) - 8, speed_track.y - 7, 16, 20 };
        char speedText[32];
        sprintf(speedText, "Speed: %d ops/sec", target_speed);
        DrawText(speedText, speed_track.x, speed_track.y - 20, 16, LIGHTGRAY);
        DrawRectangleRec(speed_knob, is_dragging_speed ? WHITE : LIGHTGRAY);
        Rectangle speed_hitBox = { speed_track.x - 20, speed_track.y - 20, speed_track.width + 40, speed_track.height + 40 };
        
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && (CheckCollisionPointRec(mouse, speed_hitBox) || is_dragging_speed)) {
            is_dragging_speed = true;
            float normalized = (mouse.x - speed_track.x) / speed_track.width;
            if (normalized < 0.0f) normalized = 0.0f;
            if (normalized > 1.0f) normalized = 1.0f;
            int calculated_speed = MIN_SPEED + (int)(normalized * (MAX_SPEED - MIN_SPEED));
            if (calculated_speed != target_speed) {
                target_speed = calculated_speed;
                SetTargetFPS(target_speed > 60 ? target_speed : 60); 
                step_timer = 0.0f; 
            }
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) is_dragging_speed = false;


       // render context on screen
        mtx_lock(&active_context->mutex);

        if (current_mode == MODE_SORTING) {
            float bar_width = (float)SCREEN_WIDTH / sort_ctx.size;
            float max_bar_height = SCREEN_HEIGHT - UI_HEIGHT - 20; 
            float height_multiplier = max_bar_height / MAX_VAL;

            for (size_t i = 0; i < sort_ctx.size; i++) {
                float bar_height = sort_ctx.array[i] * height_multiplier;
                float x_pos = i * bar_width;
                float y_pos = SCREEN_HEIGHT - bar_height; 

                Color bar_color = WHITE;
                if (is_sweeping) {
                    if ((int)i <= sweep_index) bar_color = GetColor(0x10B981FF);
                } else if(atomic_load(&sort_ctx.base.is_finished)) {
                    bar_color = GetColor(0x10B981FF); 
                } else if (i == sort_ctx.active_index_a || i == sort_ctx.active_index_b) {
                    bar_color = GetColor(0xEF4444FF); 
                }
                DrawRectangleV((Vector2){x_pos, y_pos}, (Vector2){bar_width > 2 ? bar_width - 1 : bar_width, bar_height}, bar_color);
            }
        } 
        else if (current_mode == MODE_PATHFINDING) {
            // draw gridd
            float cell_w = (float)SCREEN_WIDTH / pf_ctx.n_columns;
            float cell_h = (float)(SCREEN_HEIGHT - UI_HEIGHT) / pf_ctx.n_rows;

            for (size_t y = 0; y < pf_ctx.n_rows; y++) {
                for (size_t x = 0; x < pf_ctx.n_columns; x++) {
                    GridNode node = pf_ctx.grid[y * pf_ctx.n_columns + x]; 
                    
                    Rectangle cell = { x * cell_w, UI_HEIGHT + y * cell_h, cell_w, cell_h };
                    
                    Color node_color = GetColor(0x374151FF); // Default Unvisited (Slate Gray)
                    
                    if (x == pf_ctx.start_x && y == pf_ctx.start_y) {
                        node_color = GetColor(0x10B981FF); // Emerald Green (Start)
                    }
                    else if (x == pf_ctx.target_x && y == pf_ctx.target_y) {
                        node_color = GetColor(0x8B5CF6FF); // Vibrant Purple (Target)
                    }
                    else if (node.state == STATE_PATH) {
                        node_color = GetColor(0xFBBF24FF); // Golden Yellow (Path)
                    }
                    else if (node.is_blocked) {
                        node_color = GetColor(0x111827FF); // Deep Black/Gray (Wall)
                    }
                    else if ( node.is_heavier && node.state == STATE_FRONTIER) {
                        node_color = GetColor(0xF43F5EFF); //(Vibrant Rose/Coral)
                    }
                    else if (node.state == STATE_FRONTIER) {
                        node_color = GetColor(0x3B82F6FF); // Bright Blue (Currently exploring)
                    }
                    else if(node.is_heavier && node.state == STATE_VISITED){
                        node_color = GetColor(0x9F1239FF);  //(Muted Burgundy)
                    }
                    else if (node.state == STATE_VISITED) {
                        node_color = GetColor(0x1E3A8AFF); // Darker Blue (Fully explored)
                    }
                    else if(node.is_heavier){
                        node_color = GetColor(0x7F1D1DFF); //(Deep Crimson)
                    }
                    
                    
                    DrawRectangleRec(cell, node_color);
                    
                    // Draw a subtle border for grid definition
                    DrawRectangleLinesEx(cell, 1, GetColor(0x1F2937FF));
                }
            }
        }

        // Handle Thread Pacing
        bool should_step = false;
        if (target_speed >= 60) {
            should_step = true;
            if(current_mode==MODE_PATHFINDING){
               active_context->steps_per_frame = target_speed / 60;
            }
        } else {
            step_timer += GetFrameTime();
            float time_per_step = 1.0f / target_speed;
            if (step_timer >= time_per_step) {
                should_step = true;
                step_timer -= time_per_step; 
            }
        }

        if (should_step) {
            active_context->frame_consumed = true; 
            if (!atomic_load(&active_context->is_finished) && is_running) {
                cnd_signal(&active_context->condition_var); 
            }
        }

        mtx_unlock(&active_context->mutex);

        //docs overlay
        if (is_docs_shown) {
             DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.7f));
            
            // 2. The Modal Panel
            int modalWidth = 800;
            int modalHeight = 600;
            Rectangle modal = { 
                (SCREEN_WIDTH - modalWidth) / 2.0f, 
                (SCREEN_HEIGHT - modalHeight) / 2.0f, 
                modalWidth, 
                modalHeight 
            };
            
            DrawRectangleRec(modal, GetColor(0x1F2937FF)); // Dark Gray Base
            DrawRectangleLinesEx(modal, 2, GetColor(0x3B82F6FF)); // Blue border
            
            // Close Button ('X')
            Rectangle closeBtn = { modal.x + modal.width - 40, modal.y + 10, 30, 30 };
            bool closeHover = CheckCollisionPointRec(mouse, closeBtn);
            DrawText("X", closeBtn.x + 8, closeBtn.y + 5, 20, closeHover ? RED : LIGHTGRAY);
            
            if ((closeHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || IsKeyPressed(KEY_ESCAPE)) {
                is_docs_shown = false;
            }

            // --- RENDER CONTENT ---
            float padding = 30.0f;
            float startY = modal.y + padding;
            Font defaultFont = GetFontDefault();
            
            // Title
            DrawText(active_algo->name, modal.x + padding, startY, 30, WHITE);
            startY += 50;

            // Overview
            DrawText("Overview", modal.x + padding, startY, 20, GetColor(0x60A5FAFF)); // Light blue header
            startY += 25;
            drawTextWrapped(defaultFont, active_algo->docs.overview, (Vector2){modal.x + padding, startY}, 18, 1.0f, modalWidth - (padding*2), LIGHTGRAY);
            
            // Process (Skip down to allow space for overview)
            startY += 100; 
            DrawText("How it Works", modal.x + padding, startY, 20, GetColor(0x60A5FAFF));
            startY += 25;
            drawTextWrapped(defaultFont, active_algo->docs.process, (Vector2){modal.x + padding, startY}, 18, 1.0f, modalWidth - (padding*2), LIGHTGRAY);
            startY+=170;
            DrawText("*Note: Red cells mean that the weight of them is 5 times heavier", modal.x + padding, startY, 20, GetColor(0x60A5FAFF));

            // Complexity Box
            startY += 30;
            Rectangle compBox = { modal.x + padding, startY, modalWidth - (padding*2), 90 };
            DrawRectangleRec(compBox, GetColor(0x111827FF)); // Darker inset box
            
            DrawText("Complexity Analysis", compBox.x + 10, compBox.y + 10, 18, WHITE);
            
            // Time Complexity
            DrawText(TextFormat("Best: %s", active_algo->docs.time_best), compBox.x + 10, compBox.y + 40, 16, GetColor(0x10B981FF)); // Green
            DrawText(TextFormat("Avg: %s", active_algo->docs.time_avg), compBox.x + 150, compBox.y + 40, 16, GetColor(0xFBBF24FF)); // Yellow
            DrawText(TextFormat("Worst: %s", active_algo->docs.time_worst), compBox.x + 290, compBox.y + 40, 16, GetColor(0xEF4444FF)); // Red
            
            // Space Complexity
            DrawText(TextFormat("Space (Auxiliary): %s   Space(Stack): %s", active_algo->docs.space_aux_complexity,active_algo->docs.space_recur_complexity), 
                                    compBox.x + 10, compBox.y + 65, 16, GetColor(0xA78BFAFF)); // Purple
        }

        EndDrawing();
    }

    if (is_running) {
        stopAlgorithm(active_context, &worker_thread);
    }

    free(pf_ctx.grid); // Clean up the grid allocation
    
    cnd_destroy(&sort_ctx.base.condition_var);
    mtx_destroy(&sort_ctx.base.mutex);
    cnd_destroy(&pf_ctx.base.condition_var);
    mtx_destroy(&pf_ctx.base.mutex);
    
    CloseWindow();
    return 0;
}