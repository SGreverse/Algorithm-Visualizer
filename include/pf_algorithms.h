#ifndef PFALGORITHMS_H
#define PFALGORITHMS_H
#include "pf_types.h"

extern const Algorithm BFSAlgorithm;
extern const Algorithm DijkstraAlgorithm;
extern const Algorithm AstarAlgorithm;

void hook_MarkNode(PFContext* ctx, GridNode* node, NodeState new_state);


#endif