/* 046267 Computer Architecture - Spring 2020 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include <iostream>
using namespace std;
#include "bp_api.h"
#include <math.h>   

#define ADDR_SIZE 32
#define IDX_LSB 3

enum FSM{ SNT, WNT, WT, ST};

int bit_extraction(uint32_t pc, int k, int lsb);

class btb{

	private:

	unsigned btbSize_;
	unsigned historySize_;
	unsigned tagSize_;
	unsigned fsmState_;
	int Shared_;

	int* history_table_;
	int mask;
	FSM** fsm_table_;
	int* tag_table_;
	uint32_t* target_table_;




	public:
	unsigned btb_idx_size_; //in bits.
	bool isGlobalHist_;
	bool isGlobalTable_;
	btb(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared);
	~btb();
	unsigned get_btbSize();
	bool pc_exists(uint32_t pc);
	void pc_insert(uint32_t pc, uint32_t targetPc);
	uint32_t get_dst(uint32_t pc);
	void update_history(bool taken, int table_idx);
	void reset_history(int table_idx);
	void update_fsm(bool taken, int table_idx, uint32_t pc);
	void reset_fsm(int table_idx);
	bool get_predict(uint32_t pc);
	int xor_hash(int history, uint32_t pc);
	void print_btb(); //for debug only! 
	void print_fsm(); //for debug only! 

};
///////////////////////////////////////
// BTB METHODS
///////////////////////////////////////
btb::btb(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
		bool isGlobalHist, bool isGlobalTable, int Shared):btbSize_(btbSize), historySize_(historySize), tagSize_(tagSize), fsmState_(fsmState),
		Shared_(Shared),isGlobalHist_(isGlobalHist), isGlobalTable_(isGlobalTable){
	btb_idx_size_ = log2(btbSize);
	if (isGlobalHist){
		history_table_ = (int*)new int;
		*history_table_ = 0;
	}
	else{
		history_table_ = (int*)new int[btbSize];
		for(unsigned i=0; i<btbSize; i++)
			history_table_[i] = 0;
	}
	tag_table_ = (int*)new int[btbSize];
	
	target_table_ = (uint32_t*) new uint32_t*[btbSize];
	for (unsigned i = 0; i < btbSize; i++){
		tag_table_[i] = -1;
		target_table_[i] = -1;
	}		
	//if(isGlobalTable)
	//	fsm_table_ = (FSM**) new FSM*[1 << historySize];
	//else{
		fsm_table_ = (FSM**) new FSM*[btbSize];
		for(unsigned i = 0; i < btbSize; ++i)
			fsm_table_[i] = (FSM*)new FSM[1 << historySize]; // 2^historySize
		for(unsigned i = 0; i < btbSize; i++)
			for (int j = 0; j < 1 << historySize; j++)
				fsm_table_[i][j] = FSM(fsmState_);
	//print_fsm();
	//}
	mask = 0;
	for(unsigned i=0; i<historySize; i++)
		mask = (mask << 1) + 1;	
	
}

btb::~btb(){
	delete[] tag_table_;
	delete[] target_table_;
	if(isGlobalHist_)
		delete history_table_;
	else
		delete[] history_table_;
	if(isGlobalTable_)
		delete[] fsm_table_;
	else{
		for(unsigned i=0; i<btbSize_;i++)
			delete[] fsm_table_[i];
	}
}

unsigned btb::get_btbSize(){
	return btbSize_;
}

bool btb::pc_exists(uint32_t pc){ //
	int table_idx = bit_extraction(pc, btb_idx_size_, IDX_LSB);// extract the index from pc
	int tag = bit_extraction(pc, tagSize_, IDX_LSB + btb_idx_size_);// extract the tag from pc

	if(tag_table_[table_idx]==tag)
		return true;
	return false;
}

void btb::pc_insert(uint32_t pc, uint32_t targetPc){
	int table_idx = bit_extraction(pc, btb_idx_size_, IDX_LSB);// extract the index from pc
	int tag = bit_extraction(pc, tagSize_, IDX_LSB + btb_idx_size_);// extract the tag from pc
	tag_table_[table_idx] = tag;
	target_table_[table_idx] = targetPc;
}

uint32_t btb::get_dst(uint32_t pc){
	int table_idx = bit_extraction(pc, btb_idx_size_, IDX_LSB);// extract the index from pc
	uint32_t dst = target_table_[table_idx];
	return dst;
}

void btb::update_history(bool taken, int table_idx){
	*(history_table_ + table_idx) = *(history_table_+ table_idx) << 1;
	*(history_table_ + table_idx) = *(history_table_ + table_idx) & mask;
	*(history_table_ + table_idx) += taken;
}

void btb::reset_history(int table_idx){
	*(history_table_ + table_idx) = 0;
}

void btb::update_fsm(bool taken, int table_idx, uint32_t pc){	
	int x = xor_hash(history_table_[table_idx*(1-isGlobalHist_)], pc);
	FSM tmp = fsm_table_[table_idx*(1-isGlobalTable_)][x];
	switch(fsm_table_[table_idx*(1-isGlobalTable_)][x]){//*(1-isGlobalHist_)]){
	
	case SNT:
		if (taken)
			tmp = WNT;
		break;
	case WNT:
		if (taken)
			tmp = WT;
		else
			tmp = SNT;
		break;
	case WT:
		if (taken)
			tmp = ST;
		else
			tmp = WNT;
		break;
	case ST:
		if (!taken)
			tmp = WT;
		break;
	}
	fsm_table_[table_idx*(1-isGlobalTable_)][x] = tmp;//*(1-isGlobalHist_)] = tmp;
}

void btb::reset_fsm(int table_idx){
	for(int i=0; i< 1 << historySize_; i++)		
		fsm_table_[table_idx][i] = FSM(fsmState_);	
}

bool btb::get_predict(uint32_t pc){
	int table_idx = bit_extraction(pc, btb_idx_size_, IDX_LSB);// extract the index from pc	
	int x = xor_hash(history_table_[table_idx*(1-isGlobalHist_)], pc);
	FSM tmp = fsm_table_[table_idx*(1-isGlobalTable_)][x];//*(1-isGlobalHist_)];
	if(tmp == SNT || tmp == WNT)
		return false;
	return true;
		
}

int btb::xor_hash(int history, uint32_t pc){
	int x;
	switch(Shared_){
		case 0: // "not_using_share"
			return history;
		case 1: // "using_share_lsb"
			x = pc >> 2;
			x = x & mask;
			return x ^ history;
		case 2: //using_share_mid
			x = pc >> 16;
			x = x & mask;
			return x ^ history;
	}
	return -1; //we will never get here.
}

void btb::print_btb(){
	if(isGlobalHist_){
		cout << "global history= " << *history_table_ << endl;
		cout << "index\t" << "tag\t" << "  target" << endl;
		
		for(unsigned i=0; i<btbSize_; i++){
			cout << i << '\t' << tag_table_[i] << '\t' << target_table_[i] << endl;
		}
	}
	else{
		cout << "index\t" << "tag\t" << "target\t" << "history"<< endl;
		
		for(unsigned i=0; i<btbSize_; i++){
			cout << i << '\t' << tag_table_[i] << '\t' << target_table_[i] << '\t' << history_table_[i] << endl;
		}
	}
}

void btb::print_fsm(){
	if(isGlobalTable_){
		cout << "global FSM" << endl;
		for(int i=0; i<1 << historySize_; i++){
			cout << i << '\t' << fsm_table_[0][i] << '\t' << endl;
		}
	}
}
////////////////////////////
// END OF BTB METHODS
////////////////////////////
// The bit_extraction function extracts k bits starting with the lsb bit.

////////////////////////////
//functions
////////////////////////////
int bit_extraction(uint32_t pc, int k, int lsb){
	return (((1 << k) - 1) & (pc >> (lsb - 1)));
}



///////////////////////////
//global variables
////////////////////////////
btb *global_btb;
SIM_stats stats;


int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	global_btb = new btb(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
	stats.flush_num =0;
	stats.br_num = 0;
	if(isGlobalHist == false && isGlobalTable == false)
		stats.size = btbSize*(tagSize+ ADDR_SIZE + historySize - 1) + (btbSize)*(1 << historySize)*2 ;
	else if(isGlobalHist == false && isGlobalTable == true)
		stats.size = btbSize*(tagSize+ ADDR_SIZE + historySize -1) + (1 << historySize)*2;
	else if(isGlobalHist == true && isGlobalTable == false)
		stats.size = btbSize*(tagSize+ ADDR_SIZE -1) + historySize + (btbSize)*(1 << historySize)*2;
	else
		stats.size = btbSize*(tagSize+ ADDR_SIZE-1) + historySize + (1 << historySize)*2;
	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	bool predict;
	if (global_btb->pc_exists(pc) == false){
		*dst = pc+4;
		predict = false;
	}
	else{
		predict = global_btb->get_predict(pc);
		if(predict)
			*dst = global_btb->get_dst(pc);
		else
			*dst = pc+4;
	}
	return predict;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	
	stats.br_num++;
 	
	if((pred_dst != targetPc && taken) || (pred_dst != pc+4 && !taken)){
		stats.flush_num++;
	}
	
	int table_idx = bit_extraction(unsigned(pc), global_btb->btb_idx_size_, IDX_LSB);// extract the index from pc
	if (global_btb->pc_exists(pc) == false){
		global_btb->pc_insert(pc, targetPc);//insert pc_tag to btb_table
		if(global_btb->isGlobalHist_==false)
			global_btb->reset_history(table_idx);
		if(global_btb->isGlobalTable_==false){
			global_btb->reset_fsm(table_idx);
			global_btb->update_fsm(taken,table_idx, pc);
		}
		else{
			
			global_btb->update_fsm(taken,table_idx, pc); 
		}
		if(global_btb->isGlobalHist_==false){
			//global_btb->reset_history(table_idx);
			global_btb->update_history(taken,table_idx);
		}
		else{
			global_btb->update_history(taken,0); // 0 for global history.
		}
	}
	else{
		global_btb->update_fsm(taken, table_idx, pc);
		global_btb->update_history(taken, (1-global_btb->isGlobalHist_)*table_idx);
		global_btb->pc_insert(pc, targetPc);	
		//maybe update target
		
	}
	
	//global_btb->print_btb();
	//global_btb->print_fsm();
	return;
}

void BP_GetStats(SIM_stats *curStats){
	*curStats = stats;
	return;
}

