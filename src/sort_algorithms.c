#include "sort_algorithms.h"
#include "sort_types.h"
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <threads.h>

int hook_Compare(SortContext* ctx, int idx_a, int idx_b){
    mtx_lock(&ctx->mutex); 
    if (atomic_load(&ctx->kill_signal)) {
        mtx_unlock(&ctx->mutex);
        return -1;
    }

    ctx->compare_count++;

    int result = ctx->array[idx_a] - ctx->array[idx_b];
    ctx->active_index_a = idx_a; 
    ctx->active_index_b = idx_b; 
    ctx->frame_consumed = false; 
    while(!ctx->frame_consumed && !atomic_load(&ctx->kill_signal)){
        cnd_wait(&ctx->condition_var, &ctx->mutex);
    }
    mtx_unlock(&ctx->mutex);
    return result;
}

void hook_Swap(SortContext* ctx, int idx_a, int idx_b) {
    mtx_lock(&ctx->mutex); 
    if (atomic_load(&ctx->kill_signal)) {
        mtx_unlock(&ctx->mutex);
        return;
    }
    ctx->swap_count++;

    int temp = ctx->array[idx_a];
    ctx->array[idx_a] = ctx->array[idx_b];
    ctx->array[idx_b] = temp; 
    ctx->active_index_a = idx_a; 
    ctx->active_index_b = idx_b; 
    ctx->frame_consumed = false; 
    while (!ctx->frame_consumed && !atomic_load(&ctx->kill_signal)) {
        cnd_wait(&ctx->condition_var, &ctx->mutex);
    }
    mtx_unlock(&ctx->mutex); 
}

void hook_Write(SortContext* ctx, int idx,int val){
    mtx_lock(&ctx->mutex); 
    if (atomic_load(&ctx->kill_signal)) {
        mtx_unlock(&ctx->mutex);
        return;
    }
    ctx->write_count++;
    ctx->array[idx]=val;
    ctx->active_index_a = idx;
    ctx->active_index_b=-1;
    ctx->frame_consumed = false;
    while (!ctx->frame_consumed && !atomic_load(&ctx->kill_signal)) {
        cnd_wait(&ctx->condition_var, &ctx->mutex);
    }
    mtx_unlock(&ctx->mutex); 
}

#pragma region Bubble Sort

void bubbleSort_Init(SortContext* ctx) { 
    //nothing to init
}

void bubbleSort_Run(SortContext* ctx) {
    for(size_t i=0;i<ctx->size;i++){
        for(size_t j=0;j<ctx->size-i-1;j++){
            //loads the signal atomically
            if(atomic_load(&ctx->kill_signal)) return;
            ctx->active_index_a=j;
            ctx->active_index_b=j+1;
            if(ctx->compare(ctx,j,j+1)>0){
                ctx->swap(ctx,j,j+1);
            }
        }
    }
}

void bubbleSort_Cleanup(SortContext* ctx) {
    ctx->active_index_a = -1;
    ctx->active_index_b = -1;
}


const SortAlgorithm BubbleSortAlgo = {
    .name = "Bubble Sort",
    .docs = {
        .overview = "A simple comparison-based sorting algorithm that repeatedly steps through the list, compares adjacent elements, and swaps them if they are in the wrong order.",
        .process = "1. Start at the beginning of the array.\n"
                   "2. Compare the current element with the next element.\n"
                   "3. If the current is greater, swap them.\n"
                   "4. Move to the next element.\n"
                   "5. Repeat until the end is reached (the largest element 'bubbles' to the end).\n"
                   "6. Repeat the entire process for the remaining unsorted elements.",
        .time_best = "O(N)",
        .time_avg = "O(N^2)",
        .time_worst = "O(N^2)",
        .space_aux_complexity = "O(1)",
        .space_recur_complexity = "O(1)"
    },
    .init = bubbleSort_Init,
    .sort = bubbleSort_Run,
    .cleanup = bubbleSort_Cleanup
};

#pragma endregion

#pragma region Selection Sort

void selectionSort_Init(SortContext* ctx) { 
    //nothing to init
}

void selectionSort_Run(SortContext* ctx) {
    for (size_t i = 0; i < ctx->size - 1; i++) {
        size_t min_index = i;
        for (size_t j = i + 1; j < ctx->size; j++) {
            if(atomic_load(&ctx->kill_signal)) return;

            if (ctx->compare(ctx,j,min_index)<0) {
                min_index = j;
            }
        }

        if (min_index != i) {
            ctx->swap(ctx,min_index,i);
        }
    }

}

void selectionSort_Cleanup(SortContext* ctx) {
    ctx->active_index_a = -1;
    ctx->active_index_b = -1;
}

const SortAlgorithm SelectionSortAlgo = {
    .name = "Selection Sort",
    .docs = {
        .overview = "An in-place comparison algorithm that divides the input into a sorted and an unsorted region. It repeatedly searches the unsorted region for the minimum element and appends it to the sorted region.",
        .process = "1. Treat the entire array as the unsorted sublist.\n"
                   "2. Iterate through the unsorted sublist to find the absolute smallest element.\n"
                   "3. Swap this smallest element with the first element of the unsorted sublist.\n"
                   "4. The sorted sublist grows by one element, and the unsorted sublist shrinks by one.\n"
                   "5. Repeat this process until the entire array is sorted.",
        .time_best = "O(N^2)",
        .time_avg = "O(N^2)",
        .time_worst = "O(N^2)",
        .space_aux_complexity = "O(1)",
        .space_recur_complexity = "O(1)"
    },
    .init = selectionSort_Init,
    .sort = selectionSort_Run,
    .cleanup = selectionSort_Cleanup
};
#pragma endregion

#pragma region Insertion Sort
void insertionSort_Init(SortContext* ctx) { 
    //nothing to init
}

void insertionSort_Run(SortContext* ctx) {
    for (int i = 1; i < ctx->size; i++) {
        int j = i;
        
        while (j > 0) {
            if (atomic_load(&ctx->kill_signal)) return;

            if (ctx->compare(ctx, j - 1, j) > 0) {
                ctx->swap(ctx, j - 1, j);
                j--;
            } else {
                break;
            }
        }
    }
}

void insertionSort_Cleanup(SortContext* ctx) {
    ctx->active_index_a = -1;
    ctx->active_index_b = -1;
}


const SortAlgorithm InsertionSortAlgo = {
    .name = "Insertion Sort",
    .docs = {
        .overview = "A simple sorting algorithm that builds the final sorted array one item at a time. It is much less efficient on large lists than more advanced algorithms, but provides excellent performance for mostly-sorted data.",
        .process = "1. Assume the first element is already sorted.\n"
                   "2. Take the next element in the array (the key).\n"
                   "3. Compare the key backwards against the elements in the sorted sub-list.\n"
                   "4. Shift elements that are greater than the key one position to the right (or swap them backwards).\n"
                   "5. Insert the key into its correct sorted position.\n"
                   "6. Repeat for all remaining elements.",
        .time_best = "O(N)",
        .time_avg = "O(N^2)",
        .time_worst = "O(N^2)",
        .space_aux_complexity = "O(1)",
        .space_recur_complexity = "O(1)"
    },
    .init = insertionSort_Init,
    .sort = insertionSort_Run,
    .cleanup = insertionSort_Cleanup
};

#pragma endregion

#pragma region Merge Sort

void mergeSort_Init(SortContext* ctx) { 
    ctx->auxiliary_memory=malloc(ctx->size*sizeof(int));
    if (ctx->auxiliary_memory == NULL) {
        atomic_store(&ctx->kill_signal, true);
    }
}


void mergeSort_Merge(SortContext* ctx,int left,int mid,int right){
    int* aux = (int*)ctx->auxiliary_memory;

    for (int k = left; k <= right; k++) {
        aux[k] = ctx->array[k];
    }

    int i = left;      
    int j = mid + 1;   
    int k = left;      

    while (i <= mid && j <= right) {
        
        if (atomic_load(&ctx->kill_signal)) return;

        if (aux[i] <= aux[j]) {
            ctx->write(ctx, k, aux[i]);
            i++;
        } else {
            ctx->write(ctx, k, aux[j]);
            j++;
        }
        k++;
    }

    while (i <= mid) {
        if (atomic_load(&ctx->kill_signal)) return;
        ctx->write(ctx, k, aux[i]);
        i++;
        k++;
    }

    while (j <= right) {
        if (atomic_load(&ctx->kill_signal)) return;
        ctx->write(ctx, k, aux[j]);
        j++;
        k++;
    }

}

static void mergeSort_Recursive(SortContext* ctx,int left, int right){
    if (left >= right) return;
    
    if (atomic_load(&ctx->kill_signal)) return;

    int mid = left + (right - left) / 2;

    mergeSort_Recursive(ctx, left, mid);
    mergeSort_Recursive(ctx, mid + 1, right);
    
    mergeSort_Merge(ctx, left, mid, right);

}
void mergeSort_Run(SortContext* ctx){
    mergeSort_Recursive(ctx, 0, ctx->size-1);
}


void mergeSort_Cleanup(SortContext* ctx){
    free(ctx->auxiliary_memory);
    ctx->active_index_a = -1;
    ctx->active_index_b = -1;
}


const SortAlgorithm MergeSortAlgo = {
    .name = "Merge Sort",
    .docs = {
        .overview = "A highly efficient, stable, divide-and-conquer algorithm. It works by continuously splitting an array in half until it cannot be further divided, then merging the sorted halves back together.",
        .process = "1. Recursively divide the unsorted list into sublists until each sublist contains only one element (a single element is inherently sorted).\n"
                   "2. Allocate an auxiliary memory buffer to hold the merged elements.\n"
                   "3. Repeatedly merge adjacent sublists to produce new, larger sorted sublists.\n"
                   "4. During the merge, compare elements from the left and right halves and copy the smaller one into the auxiliary buffer.\n"
                   "5. Copy the fully merged buffer back into the main array.",
        .time_best = "O(N log N)",
        .time_avg = "O(N log N)",
        .time_worst = "O(N log N)",
        .space_aux_complexity = "O(N)",
        .space_recur_complexity = "O(log N)"
    },
    .init = mergeSort_Init,
    .sort = mergeSort_Run,
    .cleanup = mergeSort_Cleanup
};

#pragma endregion

#pragma region Quick Sort
void quickSort_Init(SortContext* ctx){
    //nothing needed
}

static int quickSort_Partition(SortContext* ctx,int low,int high){
    int i = low;
    int j = high;

    while (i < j) {
        if (atomic_load(&ctx->kill_signal)) return low;

        while (i <= high - 1 && ctx->compare(ctx, i, low) <= 0) {
            if (atomic_load(&ctx->kill_signal)) return low;
            i++;
        }

        while (j >= low + 1 && ctx->compare(ctx, j, low) > 0) {
            if (atomic_load(&ctx->kill_signal)) return low;
            j--;
        }
        if (i < j) {
            ctx->swap(ctx,i,j);
        }
    }
    ctx->swap(ctx,low,j);
    return j;
}

static void quickSort_Recursive(SortContext* ctx,int low,int high){
    if (atomic_load(&ctx->kill_signal)) return;
    
    if (low < high) {

        int pi = quickSort_Partition(ctx, low, high);


        quickSort_Recursive(ctx, low, pi - 1);
        quickSort_Recursive(ctx, pi + 1, high);
    }
}
void quickSort_Run(SortContext* ctx){
    quickSort_Recursive(ctx, 0, ctx->size-1);
}

void quickSort_Cleanup(SortContext* ctx){
    ctx->active_index_a = -1;
    ctx->active_index_b = -1;
}
const SortAlgorithm QuickSortAlgo = {
    .name = "Quick Sort",
    .docs = {
        .overview = "A highly efficient divide-and-conquer algorithm that relies on a 'pivot' element. It partitions the array into two halves based on the pivot and recursively sorts them. In practice, it is often the fastest sorting algorithm.",
        .process = "1. Choose an element from the array to act as the 'pivot'.\n"
                   "2. Partition Phase: Reorder the array so that all elements smaller than the pivot come before it, and all elements larger come after it.\n"
                   "3. Place the pivot in its final sorted position between the two halves.\n"
                   "4. Recursively apply the same process to the left sub-array and the right sub-array.\n"
                   "5. The recursion bottoms out when sub-arrays reach a size of 1 or 0.",
        .time_best = "O(N log N)",
        .time_avg = "O(N log N)",
        .time_worst = "O(N^2)",
        .space_aux_complexity = "O(1)",
        .space_recur_complexity = "O(log N)"
    },
    .init = quickSort_Init,
    .sort = quickSort_Run,
    .cleanup = quickSort_Cleanup
};
#pragma endregion

#pragma region Counting Sort
int countingSort_GetMax(SortContext* ctx){
    int max_val=-1;
    for(size_t i=0;i<ctx->size;i++){
        if(ctx->array[i]>max_val){
            max_val=ctx->array[i];
        }
    }
    return max_val;
}
void countingSort_Init(SortContext* ctx) { 
    ctx->auxiliary_memory=calloc(countingSort_GetMax(ctx)+1,sizeof(int));
    if(ctx->auxiliary_memory==NULL){
        atomic_store(&ctx->kill_signal, true);
    }
}

void countingSort_Run(SortContext* ctx) {
    int* aux=(int*)ctx->auxiliary_memory;
    int inx=0;

    for (size_t i = 0; i < ctx->size; i++) {
        if(atomic_load(&ctx->kill_signal)) return;
        ctx->compare(ctx,i,i);// to visually show a scan of the array
        aux[ctx->array[i]]++;
    }
    int max=countingSort_GetMax(ctx);
    for(int i=0;i<=max;i++){
        while(aux[i]>0){
            if(atomic_load(&ctx->kill_signal)) return;
            ctx->write(ctx,inx,i);
            inx++;
            aux[i]--;
        }
    }


}

void countingSort_Cleanup(SortContext* ctx) {
    free(ctx->auxiliary_memory);
    ctx->active_index_a = -1;
    ctx->active_index_b = -1;
}

const SortAlgorithm CountingSortAlgo = {
    .name = "Counting Sort",
    .docs = {
        .overview = "A non-comparative integer sorting algorithm. It operates by counting the number of objects that possess distinct key values, rather than performing direct element-to-element comparisons.",
        .process = "1. Scan the main array to find the absolute maximum value (K).\n"
                   "2. Allocate an auxiliary 'histogram' array of size K + 1, initialized to zero.\n"
                   "3. Iterate through the input array, using each integer's value as an index in the histogram, and increment that count.\n"
                   "4. Iterate linearly through the histogram array.\n"
                   "5. For every count greater than zero at index 'i', overwrite the main array with the value 'i' and decrement the count.",
        .time_best = "O(N + K)",
        .time_avg = "O(N + K)",
        .time_worst = "O(N + K)",
        .space_aux_complexity = "O(K)",
        .space_recur_complexity = "O(1)"
    },
    .init = countingSort_Init,
    .sort = countingSort_Run,
    .cleanup = countingSort_Cleanup
};

#pragma endregion

#pragma region Radix Sort
void radixSort_Init(SortContext* ctx) { 
    ctx->auxiliary_memory=malloc(ctx->size*sizeof(int));
}
int RadixSort_getMax(SortContext* ctx) {
    int max_val=ctx->array[0];
    for(size_t i=1;i<ctx->size;i++){
        if(ctx->array[i]>max_val){
            max_val=ctx->array[i];
        }
    }
    return max_val;
}
void RadixSort_CountSort(SortContext* ctx, int exp) {
    int* aux=(int*)ctx->auxiliary_memory;
    int i,cnt[10] = { 0 };
    size_t j;
 
    for (j = 0; j < ctx->size; j++){
        if(atomic_load(&ctx->kill_signal)) return;
        cnt[(ctx->array[j] / exp) % 10]++;
        ctx->compare(ctx,j,j);
    }
 
    for (i = 1; i < 10; i++){
        if(atomic_load(&ctx->kill_signal)) return;
        cnt[i] += cnt[i - 1];
    }
 
    for (i = ctx->size - 1; i >= 0; i--) {
        if(atomic_load(&ctx->kill_signal)) return;
        aux[cnt[(ctx->array[i] / exp) % 10] - 1] = ctx->array[i];
        cnt[(ctx->array[i] / exp) % 10]--; 
    }

    for (j = 0; j < ctx->size; j++){
        if(atomic_load(&ctx->kill_signal)) return;
        ctx->write(ctx,j,aux[j]);
    }
}

void radixSort_Run(SortContext* ctx) {
    int m = RadixSort_getMax(ctx);

    for (int exp = 1; m / exp > 0; exp *= 10)
        RadixSort_CountSort(ctx,exp);
}

void radixSort_Cleanup(SortContext* ctx) {
    free(ctx->auxiliary_memory);
    ctx->active_index_a = -1;
    ctx->active_index_b = -1;
}


const SortAlgorithm RadixSortAlgo = {
    .name = "Radix Sort",
    .docs = {
        .overview = "A non-comparative sorting algorithm that sorts data with integer keys by grouping keys by the individual digits which share the same significant position and value.",
        .process = "1. Find the maximum number in the array to determine the total number of digits required.\n"
                   "2. Perform a stable sub-sort (usually Counting Sort) for every single digit.\n"
                   "3. Start from the least significant digit (the 1s place) and sort the array.\n"
                   "4. Move to the next significant digit (the 10s place) and sort again.\n"
                   "5. Because the sub-sort is stable, items with the same current digit retain their relative order from the previous pass, resulting in a perfectly sorted array at the end.",
        .time_best = "O(N * d)",
        .time_avg = "O(N * d)",
        .time_worst = "O(N * d)",
        .space_aux_complexity = "O(N + K)",
        .space_recur_complexity = "O(1)"
    },
    .init = radixSort_Init,
    .sort = radixSort_Run,
    .cleanup = radixSort_Cleanup
};
#pragma endregion

#pragma region Heap Sort
void heapSort_Init(SortContext* ctx) { 
    //nothing to init
}

static void heapSort_Heapify(SortContext* ctx,size_t n,size_t i){
    size_t largest = i;       
    size_t left = 2 * i + 1;  
    size_t right = 2 * i + 2;  

    if (left < n && ctx->compare(ctx,left,largest)>0) {
        largest = left;
    }

    if (right < n && ctx->compare(ctx,right,largest)>0) {
        largest = right;
    }

    if (largest != i) {
        ctx->swap(ctx,i,largest);
        heapSort_Heapify(ctx,n, largest);
    }
}
void heapSort_Run(SortContext* ctx) {
    for (int i = ctx->size / 2 - 1; i >= 0; i--) {
        if(atomic_load(&ctx->kill_signal)) return;
        heapSort_Heapify(ctx,ctx->size, i);
    }

    for (int i = ctx->size - 1; i > 0; i--) {
        if(atomic_load(&ctx->kill_signal)) return;
        ctx->swap(ctx,0,i);
        heapSort_Heapify(ctx, i,0);
    }

}

void heapSort_Cleanup(SortContext* ctx) {
    ctx->active_index_a = -1;
    ctx->active_index_b = -1;
}


const SortAlgorithm HeapSortAlgo = {
    .name = "Heap Sort",
    .docs = {
        .overview = "A highly efficient in-place sorting algorithm that divides its input into a sorted and an unsorted region by dynamically managing the unsorted region using a binary heap data structure.",
        .process = "1. Build Phase: Treat the array as a complete binary tree and organize it into a Max-Heap. The absolute largest element is now securely at the root (index 0).\n"
                   "2. Extraction Phase: Swap the root element with the last element in the unsorted region, placing it in its final sorted position.\n"
                   "3. The unsorted region shrinks by one.\n"
                   "4. Call 'Heapify' on the new root to allow the element to sift down and restore the Max-Heap property.\n"
                   "5. Repeat the extraction until the heap is empty.",
        .time_best = "O(N log N)",
        .time_avg = "O(N log N)",
        .time_worst = "O(N log N)",
        .space_aux_complexity = "O(1)",
        .space_recur_complexity = "O(log N)"
    },
    .init = heapSort_Init,
    .sort = heapSort_Run,
    .cleanup = heapSort_Cleanup
};
#pragma endregion
