#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "sort_types.h"
extern const SortAlgorithm BubbleSortAlgo;
extern const SortAlgorithm QuickSortAlgo;
extern const SortAlgorithm MergeSortAlgo;
extern const SortAlgorithm SelectionSortAlgo;
extern const SortAlgorithm InsertionSortAlgo;
extern const SortAlgorithm CountingSortAlgo;
extern const SortAlgorithm RadixSortAlgo;
extern const SortAlgorithm HeapSortAlgo;

int hook_Compare(SortContext* ctx, int idx_a, int idx_b);
void hook_Swap(SortContext* ctx, int idx_a, int idx_b) ;
void hook_Write(SortContext* ctx, int idx,int val);
#endif