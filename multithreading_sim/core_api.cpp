/* 046267 Computer Architecture - Spring 2021 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>
#include <iostream>
#define BLOCK_MODE 1
#define GRAINED_MODE 0


using namespace std;


///////////////////////////////////
// Structs.
///////////////////////////////////

typedef struct thread_ {
	tcontext register_file;
	int busy_count;
	int isHalt;
	uint32_t cur_line;
} thread;


///////////////////////////////////
// Function declerations.
///////////////////////////////////

int rr(int cur_thread, thread* thread_group, int mode);
int simDone(thread* thread_group);
void inst_exe(Instruction inst, int tid, thread* thread_group);
void decreaseBusy(thread* thread_group);

// Debug functions

void print_threadsishalt(thread* thread_group);
void print_threadsbusy(thread* thread_group);
void print_thread_regitsers(thread* thread_group, int tid);
///////////////////////////////////
// Global declerations.
///////////////////////////////////

thread* block_thread_group;
thread* grained_thread_group;

double block_cpi;
double fineGrained_cpi;
///////////////////////////////////
// API implementation.
///////////////////////////////////
void CORE_BlockedMT() {
	int cycles_num = 0;
	int inst_num = 0;
	// Initiate the thread group.
	int threadsNum = SIM_GetThreadsNum();
	block_thread_group = (thread*)new thread[threadsNum];
	for(int i=0; i<threadsNum; i++){
		for(int j=0; j<REGS_COUNT; j++)
			block_thread_group[i].register_file.reg[j] = 0;
		block_thread_group[i].busy_count = 0;
		block_thread_group[i].isHalt = 0;
		block_thread_group[i].cur_line = 0;
	}
	int cur_thread = 0;
	int next_thread = -1;
	int context_switch = 0;
	int context_switch_flag = 1;
	Instruction inst;
	while(1){ // iteration == cycle.
		if(simDone(block_thread_group) == 1)
			break;
		//cout << endl <<"current cycle = " << cycles_num << endl;
		//cout << "current thread = " << cur_thread << endl;
		if(context_switch > 0) // We are in a middle of context switch.
			context_switch--;
		else{ // We are NOT in a middle of context switch.
			if(context_switch_flag != 1) // Context switch just ended. New thread was brought.
				next_thread = rr(cur_thread, block_thread_group, BLOCK_MODE);
			context_switch_flag = 0;
			if(next_thread != cur_thread && next_thread != -1){ // starting context switch.
				context_switch = SIM_GetSwitchCycles() -1;
				cur_thread = next_thread;
				context_switch_flag = 1;
			}
			else{	
				if (block_thread_group[cur_thread].busy_count == 0 && block_thread_group[cur_thread].isHalt == 0){
					inst_num++;
					//cout << "inst_exe" << endl;
					SIM_MemInstRead(block_thread_group[cur_thread].cur_line, &inst, cur_thread);
					inst_exe(inst, cur_thread, block_thread_group);
					//print_threadsishalt(block_thread_group);
					
				}
			}
		}
		decreaseBusy(block_thread_group);
		//print_threadsbusy(block_thread_group);
		//print_thread_regitsers(block_thread_group, cur_thread);
		cycles_num++;
	}
	block_cpi = cycles_num / double(inst_num);
}

void CORE_FinegrainedMT() {
	int cycles_num = 0;
	int inst_num = 0;
	// Initiate the thread group.
	int threadsNum = SIM_GetThreadsNum();
	grained_thread_group = (thread*)new thread[threadsNum];
	for(int i=0; i<threadsNum; i++){
		for(int j=0; j<REGS_COUNT; j++)
			grained_thread_group[i].register_file.reg[j] = 0;
		grained_thread_group[i].busy_count = 0;
		grained_thread_group[i].isHalt = 0;
		grained_thread_group[i].cur_line = 0;
	}
	int cur_thread = 0;
	Instruction inst;
	while(1){ // iteration == cycle.
		if(simDone(grained_thread_group) == 1)
			break;
		//cout << endl <<"current cycle = " << cycles_num << endl;
		//cout << "current thread = " << cur_thread << endl;
		if (grained_thread_group[cur_thread].busy_count == 0 && grained_thread_group[cur_thread].isHalt == 0){
			inst_num++;
			SIM_MemInstRead(grained_thread_group[cur_thread].cur_line, &inst, cur_thread);
			inst_exe(inst, cur_thread, grained_thread_group);
		}
		decreaseBusy(grained_thread_group);
		cur_thread = rr((cur_thread+1)%threadsNum, grained_thread_group, GRAINED_MODE);	
		cycles_num++;
	}
	fineGrained_cpi = cycles_num / double(inst_num);
}

double CORE_BlockedMT_CPI(){
	delete block_thread_group;
	return block_cpi;
	//return 0;
}

double CORE_FinegrainedMT_CPI(){
	delete grained_thread_group;
	return fineGrained_cpi;
	//return 0;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
	for(int i = 0; i < REGS_COUNT; i++)
		context[threadid].reg[i] = block_thread_group[threadid].register_file.reg[i];
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
	for(int i = 0; i < REGS_COUNT; i++){
			context[threadid].reg[i] = grained_thread_group[threadid].register_file.reg[i];
	}
}

////////////////////////////////
// Helper functions implementation.
////////////////////////////////
int rr(int cur_thread, thread* thread_group, int mode){
	int threadsNum = SIM_GetThreadsNum();
	//cout << "cur_thread =" << cur_thread << endl;
	//cout << "threadsNum =" << threadsNum << endl;
	int i = cur_thread;
	do{
		if(thread_group[i].busy_count == 0 && thread_group[i].isHalt == 0)
			return i;
		i = (i+1)%threadsNum;
	}
	while(i != cur_thread);
	if (mode == BLOCK_MODE){
		return cur_thread;
	}
	else
		return (cur_thread-1 + threadsNum)%threadsNum;
	return -1;
}
int simDone(thread* thread_group){
	int threadsNum = SIM_GetThreadsNum();
	for(int i = 0; i < threadsNum; i++){
		if(thread_group[i].isHalt == 0) // not finished.
			return 0;
	}
	return 1;
}

void inst_exe(Instruction inst, int tid, thread* thread_group){
	cmd_opcode opcode = inst.opcode;
	//cout << "opcode - " << opcode << endl;
	int dst_index = inst.dst_index;
	int src1_index = inst.src1_index;
	int src2_index_imm = inst.src2_index_imm;
	bool isSrc2Imm = inst.isSrc2Imm;
	uint32_t addr = 0;
	switch (opcode){
		case CMD_NOP:
			break;
		case CMD_ADD:
			thread_group[tid].register_file.reg[dst_index] = thread_group[tid].register_file.reg[src1_index] + thread_group[tid].register_file.reg[src2_index_imm];
			break;
		case CMD_SUB:
			thread_group[tid].register_file.reg[dst_index] = thread_group[tid].register_file.reg[src1_index] - thread_group[tid].register_file.reg[src2_index_imm];
			break;
		case CMD_ADDI:
			thread_group[tid].register_file.reg[dst_index] = thread_group[tid].register_file.reg[src1_index] + src2_index_imm;
			break;
		case CMD_SUBI:
			thread_group[tid].register_file.reg[dst_index] = thread_group[tid].register_file.reg[src1_index] - src2_index_imm;
			break;
		case CMD_LOAD:
			int32_t dst;
			if(isSrc2Imm)
				addr = (thread_group[tid].register_file.reg[src1_index] + src2_index_imm);//*4; //we assume the inputs is ints, so we allinged it to address.	
			else
				addr = (thread_group[tid].register_file.reg[src1_index] + thread_group[tid].register_file.reg[src2_index_imm]);//*4; //we assume the inputs is ints, so we allinged it to address.	
			SIM_MemDataRead(addr, &dst);
			thread_group[tid].register_file.reg[dst_index] = dst;				
			thread_group[tid].busy_count = SIM_GetLoadLat() + 1;
			break;
		case CMD_STORE:
			if(isSrc2Imm)
				addr = (thread_group[tid].register_file.reg[dst_index] + src2_index_imm);//*4; //we assume the inputs is ints, so we allinged it to address.	
			else
				addr = (thread_group[tid].register_file.reg[dst_index] + thread_group[tid].register_file.reg[src2_index_imm]);//*4; //we assume the inputs is ints, so we allinged it to address.	
			
			SIM_MemDataWrite(addr, (int32_t) thread_group[tid].register_file.reg[src1_index]);
			thread_group[tid].busy_count = SIM_GetStoreLat() + 1;
			break;
		case CMD_HALT:
			thread_group[tid].isHalt = 1;
			break;
		default:
		return;
	}
	thread_group[tid].cur_line++;
	return;
}

void decreaseBusy(thread* thread_group){
	int threadsNum = SIM_GetThreadsNum();
	for(int i = 0; i < threadsNum; i++){
		if(thread_group[i].busy_count > 0)
			thread_group[i].busy_count--;
	}
}

void print_threadsishalt(thread* thread_group){
	int threadsNum = SIM_GetThreadsNum();
	for(int i = 0; i < threadsNum; i++){
		cout << "thread id:" << i << "\t" << "Halt = " << thread_group[i].isHalt << endl;
	}
}

void print_threadsbusy(thread* thread_group){
	int threadsNum = SIM_GetThreadsNum();
	for(int i = 0; i < threadsNum; i++){
		cout << "thread id:" << i << "\t" << "busy = " << thread_group[i].busy_count << endl;
	}
}

void print_thread_regitsers(thread* thread_group, int tid){
	cout << "register file of thread: " << tid << endl;
	for(int i =0; i< REGS_COUNT; i++)
		cout << 'R'<< i << '\t' << thread_group[tid].register_file.reg[i] << "\t";
	cout << endl;
}