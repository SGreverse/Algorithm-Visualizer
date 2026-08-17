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
#include <math.h>

#define MAX_VAL 400
#define SCREEN_WIDTH 1000  // Widened slightly for better tab fit
#define SCREEN_HEIGHT 700

// --- UI AND ARRAY CONSTANTS ---
#define UI_HEIGHT 130
#define MAX_ARRAY_SIZE 1000
#define MIN_ARRAY_SIZE 10

const SortAlgorithm* ALGORITHMS[] = {
    &BubbleSortAlgo,
    &InsertionSortAlgo,
    &SelectionSortAlgo,
    &QuickSortAlgo,
    &MergeSortAlgo,
    &CountingSortAlgo,
    &RadixSortAlgo,
    &HeapSortAlgo
};

const SortAlgorithm* active_algo = &BubbleSortAlgo; 


float current_frequency = 0.0f;
float audio_phase = 0.0f;

void audioCallback(void *bufferData, unsigned int frames) {
    short *buffer = (short *)bufferData;
    
    for (unsigned int i = 0; i < frames; i++) {
        if (current_frequency > 0.0f) {
            // Generate a simple sine wave
            audio_phase += 2.0f * PI * current_frequency / 44100.0f;
            if (audio_phase > 2.0f * PI) audio_phase -= 2.0f * PI;
            
            // Multiply by 8000 for a comfortable volume level
            buffer[i] = (short)(sinf(audio_phase) * 8000.0f); 
        } else {
            buffer[i] = 0; // Silence
        }
    }
}


int WorkerThread_Run(void* arg) {
    SortContext* ctx = (SortContext*)arg;
    active_algo->init(ctx);
    active_algo->sort(ctx);
    
    // Only flag as sorted if we weren't killed mid-run
    if (!atomic_load(&ctx->kill_signal)) {
        atomic_store(&ctx->is_sorted, true);
    }
    active_algo->cleanup(ctx);
    return 0;
}

void RandomizeArray(SortContext* ctx) {
    for (size_t i = 0; i < ctx->size; i++) {
        ctx->array[i] = (rand() % MAX_VAL) + 10; 
    }
    atomic_store(&ctx->is_sorted, false);
    
    // Reset the counters
    ctx->compare_count = 0;
    ctx->swap_count = 0;
    ctx->write_count = 0;
}

void StopSorting(SortContext* ctx, thrd_t* worker_thread) {
    atomic_store(&ctx->kill_signal, true);
    
    mtx_lock(&ctx->mutex);
    ctx->frame_consumed = true; 
    cnd_broadcast(&ctx->condition_var);
    mtx_unlock(&ctx->mutex);

    thrd_join(*worker_thread, NULL); // wait for thread to die 
}

void StartSorting(SortContext* ctx, thrd_t* worker_thread) {
    atomic_store(&ctx->kill_signal, false);
    atomic_store(&ctx->is_sorted, false);
    ctx->active_index_a = -1;
    ctx->active_index_b = -1;
    thrd_create(worker_thread, WorkerThread_Run, ctx);
}

void DrawTextWrapped(Font font, const char* text, Vector2 position, float fontSize, float spacing, float maxWidth, Color tint) {
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

    InitAudioDevice();
    bool is_audio_ready = IsAudioDeviceReady();
    AudioStream stream = { 0 };

    if (is_audio_ready) {
        SetAudioStreamBufferSizeDefault(4096);
        stream = LoadAudioStream(44100, 16, 1);
        SetAudioStreamCallback(stream, audioCallback);
        PlayAudioStream(stream);
    } else {
        TraceLog(LOG_WARNING, "Audio hardware not found. Running visualizer in silent mode.");
    }

    // allocate max possible memory upfront to prevent re-allocating during runtime
    int data_array[MAX_ARRAY_SIZE];
    
    SortContext context = {0};
    context.array = data_array;
    context.size = 150; // Default starting size
    context.swap = hook_Swap;
    context.write = hook_Write;
    context.compare = hook_Compare;

    atomic_init(&context.is_sorted, false);
    atomic_init(&context.kill_signal, false);
    mtx_init(&context.mutex, mtx_plain);
    cnd_init(&context.condition_var);

    RandomizeArray(&context);

    thrd_t worker_thread;
    bool is_running = false;
    bool is_dragging_slider = false;
    bool is_docs_shown = false;
    bool is_sweeping = false;
    float sweep_timer = 0.0f;
    int sweep_index = -1;
    const float SWEEP_DURATION = 1.5f;

    while (!WindowShouldClose()) { 
        Vector2 mouse = GetMousePosition();

        if (is_running && atomic_load(&context.is_sorted)) {
            thrd_join(worker_thread, NULL);
            is_running = false;
            
            // Trigger the Green Sweep animation instead of instant finish
            is_sweeping = true;
            sweep_timer = 0.0f;
            sweep_index = 0;
        }

        if (is_sweeping) {
            sweep_timer += GetFrameTime();
            sweep_index = (int)((sweep_timer / SWEEP_DURATION) * context.size);
            
            if (sweep_index >= context.size) {
                is_sweeping = false;
                sweep_index = context.size;
                current_frequency = 0.0f; // Silence
            } else {
                // Play sound based on the sweeping index
                float val = context.array[sweep_index];
                current_frequency = 120.0f + (val / MAX_VAL) * 1000.0f;
            }
        } else if (is_running && context.active_index_a != -1) {
            // Play sound based on the active algorithmic pointer
            float val = context.array[context.active_index_a];
            current_frequency = 120.0f + (val / MAX_VAL) * 1000.0f;
        } else {
            current_frequency = 0.0f; // Silence if idle
        }

        BeginDrawing();
        ClearBackground(GetColor(0x121212FF)); // Sleek dark gray background


        DrawRectangle(0, 0, SCREEN_WIDTH, UI_HEIGHT, GetColor(0x1F2937FF)); // UI Panel background
        DrawLine(0, UI_HEIGHT, SCREEN_WIDTH, UI_HEIGHT, GetColor(0x374151FF)); // Panel border

        //  Algorithm Navigation Bar
        int num_algos = sizeof(ALGORITHMS) / sizeof(ALGORITHMS[0]);
        float tab_width = (float)SCREEN_WIDTH / num_algos;
        
        for (int i = 0; i < num_algos; i++) {
            Rectangle tab = { i * tab_width, 0, tab_width, 40 };
            bool is_hover = CheckCollisionPointRec(mouse, tab);
            bool is_active = (active_algo == ALGORITHMS[i]);

            // Tab Colors
            if (is_active) DrawRectangleRec(tab, GetColor(0x3B82F6FF)); // Blue
            else if (is_hover) DrawRectangleRec(tab, GetColor(0x4B5563FF)); // Light Gray
            else DrawRectangleRec(tab, GetColor(0x1F2937FF)); // Dark Gray

            DrawRectangleLinesEx(tab, 1, GetColor(0x111827FF));
            
            // Center text
            int text_width = MeasureText(ALGORITHMS[i]->name, 10);
            DrawText(ALGORITHMS[i]->name, tab.x + (tab_width / 2) - (text_width / 2), tab.y + 15, 10, WHITE);

            // Tab Click Logic (Only allow switching if NOT running)
            if (!is_docs_shown && is_hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !is_running) {
                active_algo = ALGORITHMS[i];
                RandomizeArray(&context);
            }
        }

        // B. Main Control Button (Start / Stop / Reset)
        Rectangle btn = { 30, 60, 160, 50 };
        bool btn_hover = CheckCollisionPointRec(mouse, btn);
        
        if (!is_running && !atomic_load(&context.is_sorted)) {
            // START STATE
            DrawRectangleRec(btn, btn_hover ? GetColor(0x10B981FF) : GetColor(0x059669FF)); // Green
            DrawText("START", btn.x + 45, btn.y + 15, 20, WHITE);
            if (btn_hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                StartSorting(&context, &worker_thread);
                is_running = true;
            }
        } 
        else if (is_running) {
            // STOP STATE
            DrawRectangleRec(btn, btn_hover ? GetColor(0xEF4444FF) : GetColor(0xDC2626FF)); // Red
            DrawText("STOP", btn.x + 55, btn.y + 15, 20, WHITE);
            if (btn_hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                StopSorting(&context, &worker_thread);
                is_running = false;
            }
        }
        else if (atomic_load(&context.is_sorted)) {
            // RESET STATE
            DrawRectangleRec(btn, btn_hover ? GetColor(0x8B5CF6FF) : GetColor(0x7C3AEDFF)); // Purple
            DrawText("RESET", btn.x + 45, btn.y + 15, 20, WHITE);
            if (btn_hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                RandomizeArray(&context);
                context.active_index_a = -1;
                context.active_index_b = -1;
            }
        }
        Rectangle info_btn={220,60,160,50};
        bool info_btn_hover = CheckCollisionPointRec(mouse, info_btn);
        DrawRectangleRec(info_btn, BLACK); // Black
        DrawText("DOCS", info_btn.x + 45, info_btn.y + 15, 20, WHITE);
        if (info_btn_hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            is_docs_shown=true;
        }

        // Array Size Slider
        Rectangle track = { 400, 85, 300, 6 };
        DrawRectangleRec(track, GetColor(0x4B5563FF));
        
        // Calculate knob position based on current size
        float percent = (float)(context.size - MIN_ARRAY_SIZE) / (MAX_ARRAY_SIZE - MIN_ARRAY_SIZE);
        Rectangle knob = { track.x + (percent * track.width) - 8, track.y - 7, 16, 20 };
        
        // Slider Text
        char sizeText[32];
        sprintf(sizeText, "Array Size: %zu", context.size);
        DrawText(sizeText, track.x, track.y - 25, 20, LIGHTGRAY);

        // Slider Interaction Logic (Disabled while sorting)
        if (!is_running) {
            DrawRectangleRec(knob, is_dragging_slider ? WHITE : LIGHTGRAY);
            
            Rectangle hitBox = { track.x - 20, track.y - 20, track.width + 40, track.height + 40 };
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && (CheckCollisionPointRec(mouse, hitBox) || is_dragging_slider)) {
                is_dragging_slider = true;
                
                // Calculate new size based on mouse X
                float normalized = (mouse.x - track.x) / track.width;
                if (normalized < 0.0f) normalized = 0.0f;
                if (normalized > 1.0f) normalized = 1.0f;
                
                size_t calculated_size = MIN_ARRAY_SIZE + (size_t)(normalized * (MAX_ARRAY_SIZE - MIN_ARRAY_SIZE));
                
                if (calculated_size != context.size) {
                    context.size = calculated_size;
                    RandomizeArray(&context); 
                }
            }
        } else {
            DrawRectangleRec(knob, GetColor(0x374151FF)); // Disabled color
        }
        
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) is_dragging_slider = false;

        int stats_x = SCREEN_WIDTH - 280;
        int stats_y = 55;
        
        DrawText(TextFormat("Compares: %zu", context.compare_count), stats_x, stats_y, 20, GetColor(0x9CA3AFFF)); // Light Gray
        DrawText(TextFormat("Swaps: %zu", context.swap_count), stats_x, stats_y + 25, 20, GetColor(0x9CA3AFFF)); 
        DrawText(TextFormat("Writes: %zu", context.write_count), stats_x, stats_y + 50, 20, GetColor(0x9CA3AFFF));
        //draw array
        //requiresgetting the lock
        mtx_lock(&context.mutex);

        float bar_width = (float)SCREEN_WIDTH / context.size;
        
        float max_bar_height = SCREEN_HEIGHT - UI_HEIGHT - 20; 
        float height_multiplier = max_bar_height / MAX_VAL;

        for (size_t i = 0; i < context.size; i++) {
            float bar_height = context.array[i] * height_multiplier;
            float x_pos = i * bar_width;
            float y_pos = SCREEN_HEIGHT - bar_height; 

            Color bar_color = WHITE;
            if (is_sweeping) {
                // Paint green if the sweep index has passed this bar
                if ((int)i <= sweep_index) bar_color = GetColor(0x10B981FF);
            }
            else if(atomic_load(&context.is_sorted)){
                bar_color = GetColor(0x10B981FF); // Emerald Green
            }
            else if (i == context.active_index_a || i == context.active_index_b) {
                bar_color = GetColor(0xEF4444FF); // Vibrant Red
            }

            DrawRectangleV((Vector2){x_pos, y_pos}, (Vector2){bar_width > 2 ? bar_width - 1 : bar_width, bar_height}, bar_color);
        }

        //signal algorithm thread to keep going
        context.frame_consumed = true;

        if (!atomic_load(&context.is_sorted) && is_running) {
            cnd_signal(&context.condition_var);
        }

        mtx_unlock(&context.mutex);


        if (is_docs_shown) {
            // 1. Dim the background
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
            DrawTextWrapped(defaultFont, active_algo->docs.overview, (Vector2){modal.x + padding, startY}, 18, 1.0f, modalWidth - (padding*2), LIGHTGRAY);
            
            // Process (Skip down to allow space for overview)
            startY += 100; 
            DrawText("How it Works", modal.x + padding, startY, 20, GetColor(0x60A5FAFF));
            startY += 25;
            DrawTextWrapped(defaultFont, active_algo->docs.process, (Vector2){modal.x + padding, startY}, 18, 1.0f, modalWidth - (padding*2), LIGHTGRAY);
            
            // Complexity Box
            startY += 200;
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
        StopSorting(&context, &worker_thread);
    }


    if (is_audio_ready) {
        UnloadAudioStream(stream);
        CloseAudioDevice();
    }
    cnd_destroy(&context.condition_var);
    mtx_destroy(&context.mutex);
    CloseWindow();
    return 0;
}