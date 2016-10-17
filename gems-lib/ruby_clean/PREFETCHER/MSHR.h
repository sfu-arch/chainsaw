#ifndef __MSHR_H__
#define __MSHR_H__

#include "pref_types.h"


class PrefetchMissStatusHandlingRegisterEntry{
private:
public:
  // Budget Count

  // Issued bit :  1 bit
  // Valid bit  :  1 bit
  // Address bit: 26 bit
  // Total Count: 28 bit
  bool Valid;
  bool Issue;
  bool Done;
  CacheAddr_t Addr;

public:
  // Budget() returns an amount of storage for
  // implementing prefetch algorithm.
  int Budget(){ return 0; }

  // Initialize MSHR entry
  void Init(){
    Valid = false;
    Issue = false;
    Addr  = (CacheAddr_t)0;
  }

  // Returns hit or miss with MSHR entry.
  bool Hit(CacheAddr_t addr) {
    // NULL pointer handling
    if(addr==0) return true;

    // Check Address
    addr ^= Addr;
    addr &= ~BLK_MASK;
    return Valid && (addr == (CacheAddr_t)0);
  }

  // Assign Entry
  void Entry(CacheAddr_t addr){
    Valid = true;
    Issue = false;
    Addr  = addr;
  }

  // Assign Entry
  void SetDone(){
    Valid = false;
    Issue = true;
    Done  = true;
  }

  // Assign Entry
  void IssuePrefetch(COUNTER cycle){
     ASSERT(Valid && !Issue); // ska124 --  assert triggered here
    Issue = true;

    // Issue L2 prefetch whether L2 has it or not.[AS]. Need to issue prefetch at this point.
    //       IssueL2Prefetch(cycle, Addr);
   }

  // Housekeeping for MSHR entry
  void Housekeeping(COUNTER cycle){
    // When V bit is not assigned, this entry is free.
    ASSERT(Valid);

    // When GetPrefetchBit() returns a not negative value,
    // the fill of the entry will be finished.
    // In realistic processor, this judge should not based on prefetch bit.
    // (This might be done with the setting of the VALID bit of a tag array)
    // So, it is not so high complexity.
    if(Done) {
      // Release MSHR entry
      Valid = false;
    }
  }
};

class PrefetchMissStatusHandlingRegister {
public:
  // Total Budget Size : 901 bits

  // MSHR Entry Size is 32
  int ptr; // 5 bits
  PrefetchMissStatusHandlingRegisterEntry mshr[PREF_MSHR_SIZE]; // 28 bits x 32

public:
  PrefetchMissStatusHandlingRegister() {
    ptr = 0;
    for(int i=0; i<PREF_MSHR_SIZE; i++){
      mshr[i].Init();
    }
  }

  bool Full(){
    return mshr[ptr].Valid;
  }

  bool Search(CacheAddr_t addr){
    // Search Previous Memory Access
    for(int i=0; i<PREF_MSHR_SIZE; i++){
      // When the appropriate access is found, the access is merged.
      if(mshr[i].Hit(addr)) return true;
    }
    return false;
  }

  void Done(CacheAddr_t addr){

    for(int i=0; i<PREF_MSHR_SIZE; i++){
      // When the appropriate access is found, the access is merged.
      if(mshr[i].Hit(addr)) mshr[i].SetDone();
    }
  }

  void Issue(COUNTER cycle, CacheAddr_t addr){
    // Check In-flight memory access
    if(Search(addr)) return;

    // When the appropriate access cannot be found,
    // the access will issue for main memory.
    mshr[ptr].Entry(addr);

    mshr[ptr].Housekeeping(cycle);
    ptr = (ptr + 1) % PREF_MSHR_SIZE;
  }

  void Housekeeping(COUNTER cycle){
    // housekeeping for MSHR...
    for(int i=0; i<PREF_MSHR_SIZE; i++){
      if(mshr[i].Valid){
    // This Housekeeping() can invoke
    // only when the mshr entry is already assigned.
    mshr[i].Housekeeping(cycle);
      }
    }
  }

  void PrefetchHousekeeping(COUNTER cycle){
    for(int i=0; i<PREF_MSHR_SIZE; i++){
      int p = (ptr + i + 1) % PREF_MSHR_SIZE;
      if((mshr[p].Valid) && (!mshr[p].Issue))
    // When the entry does not issued,
    // prefetch MSHR issues request to Memory Unit.
       mshr[p].IssuePrefetch(cycle);
   }
  }
};


#endif
