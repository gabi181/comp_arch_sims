/* 046267 Computer Architecture - Spring 21 - HW #3               */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#include "dflow_calc.h"
#include <algorithm>

using namespace std;


typedef struct{
	int depend1_idx;
	int depend2_idx;
	int depth;
} element;

unsigned int glob_numOfInsts = 0;

ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
    element* ctx = (element*)new element[numOfInsts];
	for(unsigned int i=0; i<numOfInsts; i++){
		ctx[i].depend1_idx=-1;
		ctx[i].depend2_idx=-1;
		ctx[i].depth=-1;
	}
		
	if(ctx == NULL)
		return PROG_CTX_NULL;
	for(unsigned int i=0; i<numOfInsts; i++){
		for(unsigned int j=0; j<i; j++){
			if((unsigned int)progTrace[j].dstIdx == progTrace[i].src1Idx) 
				ctx[i].depend1_idx = j; //do not break, take the last dependency.
			if((unsigned int)progTrace[j].dstIdx == progTrace[i].src2Idx)
				ctx[i].depend2_idx = j; //do not break, take the last dependency.
		}
	}
	int depend1_idx = -1;
	int depend2_idx = -1;
	int max_depth =-1;
	for(unsigned int i=0; i<numOfInsts; i++){
		depend1_idx = ctx[i].depend1_idx;
		depend2_idx = ctx[i].depend2_idx;
		if(depend1_idx == -1 && depend2_idx == -1)
			ctx[i].depth = opsLatency[progTrace[i].opcode];
		else if(depend1_idx != -1 && depend2_idx == -1)
			ctx[i].depth = ctx[depend1_idx].depth + opsLatency[progTrace[i].opcode];
		else if(depend1_idx == -1 && depend2_idx != -1)
			ctx[i].depth = ctx[depend2_idx].depth + opsLatency[progTrace[i].opcode];
		else{
			max_depth = std::max(ctx[depend1_idx].depth, ctx[depend2_idx].depth);
			ctx[i].depth = max_depth + opsLatency[progTrace[i].opcode];
		}
	}
	glob_numOfInsts = numOfInsts;
	return (ProgCtx)ctx;
}

void freeProgCtx(ProgCtx ctx) {
	delete[] (element*)ctx;
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    if(theInst >= 0 && theInst < glob_numOfInsts){
		element* our_ctx =  (element*)ctx;
		int depend1_idx = our_ctx[theInst].depend1_idx;
		int depend2_idx = our_ctx[theInst].depend2_idx;
		int depth;
		// return only the dependency depth. without the current depth.
		if(depend1_idx == -1 && depend2_idx == -1)
			depth = 0;
		else if(depend1_idx != -1 && depend2_idx == -1)
			depth = our_ctx[depend1_idx].depth;
		else if(depend1_idx == -1 && depend2_idx != -1)
			depth = our_ctx[depend2_idx].depth;
		else{
			depth = std::max(our_ctx[depend1_idx].depth, our_ctx[depend2_idx].depth);
		}
		return depth;
	}
	return -1;
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    if(theInst >= 0 && theInst < glob_numOfInsts){
		element* our_ctx =  (element*)ctx;
		*src1DepInst = our_ctx[theInst].depend1_idx;
		*src2DepInst = our_ctx[theInst].depend2_idx;
		return 0;
	}
	return -1;
}

int getProgDepth(ProgCtx ctx) {
    int max_depth = 0;
	element* our_ctx =  (element*)ctx;
	for(unsigned int i=0; i<glob_numOfInsts; i++){
		if(our_ctx[i].depth > max_depth)
			max_depth = our_ctx[i].depth;
	}
	return max_depth;
}


