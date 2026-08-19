#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "sort_types.h"
extern const Algorithm BubbleSortAlgo;
extern const Algorithm QuickSortAlgo;
extern const Algorithm MergeSortAlgo;
extern const Algorithm SelectionSortAlgo;
extern const Algorithm InsertionSortAlgo;
extern const Algorithm CountingSortAlgo;
extern const Algorithm RadixSortAlgo;
extern const Algorithm HeapSortAlgo;

int hook_Compare(SortContext* ctx, int idx_a, int idx_b);
void hook_Swap(SortContext* ctx, int idx_a, int idx_b) ;
void hook_Write(SortContext* ctx, int idx,int val);
#endif