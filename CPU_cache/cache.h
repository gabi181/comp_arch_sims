#include <math.h>   
#include <stdlib.h> 
#include <string>
#include <sstream>
#include <iostream>

using namespace std;
using std::string;

#define ADDR_SIZE 32
///////////////////////////////////////////////////
// helper functions
///////////////////////////////////////////////////
int bit_extraction(string addr, int k, int lsb){
	unsigned long num = strtoul(addr.c_str(), NULL, 16);
	return (((1 << k) - 1) & (num >> (lsb - 1)));
}

string set_n_tag_2_addr(int set, int tag, int set_size, int offset_size){
	set = set << offset_size;
	tag = tag << offset_size + set_size;
	std::stringstream stream;
	stream << std::hex << set + tag;
	//string result( stream.str() );
	return stream.str();
}


typedef struct cache_table{
	int tag;
	int LRU_age;
	int dirty;
}cache_table;

///////////////////////////////////////////////////
// cache class
///////////////////////////////////////////////////

class cache{

	private:

	//unsigned cycles_per_access;
	unsigned MemSize_;
	unsigned Assoc_;
	unsigned Bsize_;
	cache_table **table_;
	int offset_size_;
	int set_size_;
	int tag_size_;

	public:
	
	cache(unsigned MemSize, unsigned Assoc, unsigned Bsize);
	~cache();
	bool read(string addr, string& evac_addr, int* isDirty);
	bool writeAlloc(string addr, string& evac_addr, int* isDirty);
	bool writeNoAlloc(string addr);
	void evacuate(string addr);
	bool update_LRU(string addr);
	void print_cache_table();
};

///////////////////////////////////////////////////
// cache methods
///////////////////////////////////////////////////



cache::cache(unsigned MemSize, unsigned Assoc, unsigned Bsize):MemSize_(pow(2,MemSize)),Assoc_(pow(2,Assoc)), Bsize_(pow(2,Bsize)){
	table_ = (cache_table**) new cache_table*[MemSize_/(Assoc_*Bsize_)];
	for(int i = 0; i < int(MemSize_/(Assoc_*Bsize_)); ++i){
		table_[i] = (cache_table*)new cache_table[Assoc_];
		for(unsigned j = 0; j<Assoc_; j++){
			table_[i][j].tag = -1;
			table_[i][j].LRU_age = -1;
			table_[i][j].dirty = 0;
		}
	}
	offset_size_ = log2(Bsize_);
	set_size_ = log2(MemSize_/(Assoc_*Bsize_));
	tag_size_ = ADDR_SIZE - set_size_ - offset_size_;
	
	//cout << "created cache" << endl;
}

cache::~cache(){
	for(int i=0; i < int(MemSize_/(Assoc_*Bsize_));i++){
		delete[] table_[i];
	}
}

bool cache::read(string addr, string& evac_addr, int* isDirty){
	int set = bit_extraction(addr, set_size_, offset_size_ + 1);
	int tag = bit_extraction(addr, tag_size_, set_size_ + offset_size_ + 1);
	int age =-2;
	int oldest_idx=-1;
	int dirty = 0;
	bool hit = update_LRU(addr);
	if(hit == true)
		return true;
	// missed.
	for(int i=0; i<Assoc_; i++){ 
		if(table_[set][i].LRU_age > age || table_[set][i].LRU_age == -1){ // find oldest for evac.
			age = table_[set][i].LRU_age;
			oldest_idx = i;
			dirty = table_[set][i].dirty;
			if(age == -1) //we found empty block.
				break;
		}
	}
	evac_addr = set_n_tag_2_addr(set, table_[set][oldest_idx].tag, set_size_, offset_size_);
	*isDirty = dirty;
	if(age == -1)// we didnt evac.
		evac_addr = "";
	table_[set][oldest_idx].LRU_age = 0;
	table_[set][oldest_idx].tag = tag;
	table_[set][oldest_idx].dirty = 0;
	//print_cache_table();
	//cout << "dirty =" << table_[set][oldest_idx].dirty << endl;
	return false;
	
}

void cache::evacuate(string addr){
	int set = bit_extraction(addr, set_size_, offset_size_ + 1);
	int tag = bit_extraction(addr, tag_size_, set_size_ + offset_size_ + 1);
	for(int i=0; i<Assoc_; i++){
		if(table_[set][i].tag == tag){
			table_[set][i].tag = -1;
			table_[set][i].LRU_age = -1;
			table_[set][i].dirty = 0;
			}
	}
}

bool cache::update_LRU(string addr){
	int set = bit_extraction(addr, set_size_, offset_size_ + 1);
	int tag = bit_extraction(addr, tag_size_, set_size_ + offset_size_ + 1);
	for(int i=0; i<Assoc_; i++){ 
		if(table_[set][i].LRU_age != -1) //if not clear
			table_[set][i].LRU_age++; // getting older.
	}
	for(int i=0; i<Assoc_; i++){
		if(table_[set][i].tag == tag){ // if hit.
			table_[set][i].LRU_age = 0; // getting younger.
			return true; // hit
		}
	}
	return false; // miss
}

bool cache::writeNoAlloc(string addr){
	int set = bit_extraction(addr, set_size_, offset_size_ + 1);
	int tag = bit_extraction(addr, tag_size_, set_size_ + offset_size_ + 1);
	if(update_LRU(addr) == true){ //if hit 
		for(int i=0; i<Assoc_; i++){
			if(table_[set][i].tag == tag){ // if hit.
				table_[set][i].dirty = 1; // dirty.
				return true; // hit
			}
		}
	}
	return false;
	
}

bool cache::writeAlloc(string addr, string& evac_addr, int* isDirty){
	int set = bit_extraction(addr, set_size_, offset_size_ + 1);
	int tag = bit_extraction(addr, tag_size_, set_size_ + offset_size_ + 1);
	bool hit = read(addr, evac_addr, isDirty);

	for(int i=0; i<Assoc_; i++){
		if(table_[set][i].tag == tag){ // if hit.
			table_[set][i].dirty = 1; // dirty.
		}
	}
	return hit;
	
}

void cache::print_cache_table(){
	for(int i = 0; i < int(MemSize_/(Assoc_*Bsize_)); ++i){
		cout << "floor:" << i << endl;
		cout << "room" << "\t" << "tag" << "\t" << "LRU_age" << "\t" << "dirty" << endl;
		for(unsigned j = 0; j<Assoc_; j++){
			cout << j << "\t|\t" << table_[i][j].tag << "\t|\t" << table_[i][j].LRU_age << "\t|\t" << table_[i][j].dirty << endl;
		}
	}
}
///////////////////////////////////////////////////
// mem_hier class
///////////////////////////////////////////////////

class mem_hier{
	private:
	
	cache L1_;
	cache L2_;
	int MemCyc_;
	int L1Cyc_;
	int L2Cyc_;
	int WrAlloc_;
	int total_cycles_;
	int L1Miss_;
	int L2Miss_;
	int L1Acc_;
	int L2Acc_;
	
	public:
	mem_hier(unsigned L1Size, unsigned L2Size, unsigned L1Assoc, unsigned L2Assoc, unsigned BSize ,unsigned MemCyc,unsigned L1Cyc, unsigned L2Cyc, unsigned WrAlloc);
	void read(string addr);
	void write(string addr);
	int gettotal_cycles();
	int getL1Miss();
	int getL2Miss();
	int getL1Acc();
	int getL2Acc();
	void print_caches();
	
};



///////////////////////////////////////////////////
// mem_hier methods
///////////////////////////////////////////////////
mem_hier::mem_hier(unsigned L1Size, unsigned L2Size, unsigned L1Assoc, unsigned L2Assoc, unsigned BSize ,unsigned MemCyc,unsigned L1Cyc, unsigned L2Cyc, unsigned WrAlloc): L1_(L1Size, L1Assoc, BSize), L2_(L2Size, L2Assoc, BSize), MemCyc_(MemCyc), L1Cyc_(L1Cyc), L2Cyc_(L2Cyc), WrAlloc_(WrAlloc){
	//cout << "entered mem hierch read" << endl;
	total_cycles_ = 0;
	L1Miss_ = 0;
	L2Miss_ = 0;
	L1Acc_ = 0;
	L2Acc_ = 0;
}

void mem_hier::read(string addr){
	//cout << "entered mem hierch read" << endl;
	string L1evac_addr;
	string L2evac_addr;
	int L1isDirty =0;
	int L2isDirty =0;
	L1Acc_++;
	total_cycles_ += L1Cyc_;
	if (L1_.read(addr, L1evac_addr, &L1isDirty) == false){
		L1Miss_++;
		L2Acc_++;
		total_cycles_ += L2Cyc_;
		if(L2_.read(addr, L2evac_addr, &L2isDirty) == false){
			L2Miss_++;
			total_cycles_ += MemCyc_;
			if(L2evac_addr != "")
				L1_.evacuate(L2evac_addr);
		}
		if(L1isDirty == true){
			//cout << "isDirty is true" << endl; 
			L2_.update_LRU(L1evac_addr);
		}
	}
	//L1_.print_cache_table();
	//cout << "after mem hierch read" << endl;
}

void mem_hier::write(string addr){
	string L1evac_addr;
	string L2evac_addr;
	int L1isDirty =0;
	int L2isDirty =0;
	if(WrAlloc_ == 0){ // write no allocate.
		L1Acc_++;
		total_cycles_ += L1Cyc_;
		if(L1_.writeNoAlloc(addr) == false){
			L1Miss_++;
			L2Acc_++;
			total_cycles_ += L2Cyc_;
			if(L2_.writeNoAlloc(addr) == false){
				L2Miss_++;
				total_cycles_ += MemCyc_;
			}
		}
	}
	else{// write allocate.
		L1Acc_++;
		total_cycles_ += L1Cyc_;
		if(L1_.writeAlloc(addr, L1evac_addr, &L1isDirty) == false){
			L1Miss_++;
			L2Acc_++;
			total_cycles_ += L2Cyc_;
			if(L2_.read(addr, L2evac_addr, &L2isDirty) == false){ // read because we dont want to dirty the block.
				L2Miss_++;
				total_cycles_ += MemCyc_;
				if(L2evac_addr != "")
					L1_.evacuate(L2evac_addr);
			}
			if(L1isDirty == true){
				L2_.update_LRU(L1evac_addr);
			}
		}
	}
}

void mem_hier::print_caches(){
	cout << "-----L1-----" << endl;
	L1_.print_cache_table();
	cout << "-----L2-----" << endl;
	L2_.print_cache_table();
}

int mem_hier::gettotal_cycles(){
	return total_cycles_;
}
int mem_hier::getL1Miss(){
	return L1Miss_;
}
int mem_hier::getL2Miss(){
	return L2Miss_;
}
int mem_hier::getL1Acc(){
	return L1Acc_;
}
int mem_hier::getL2Acc(){
	return L2Acc_;
}
