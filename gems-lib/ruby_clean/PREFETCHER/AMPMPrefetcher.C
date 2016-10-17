

#include "AMPMPrefetcher.h"
#include "GenericPrefetcher.h"
#include "pref_types.h"
#include "Global.h"
#include "Map.h"
#include "Address.h"
#include "Profiler.h"
#include "EventQueue.h"
#include "AbstractChip.h"
#include <iostream>
#include "Protocol.h"
#include "Chip.h"
#include "RubyConfig.h"
#include "System.h"
#include "Types.h"
#include <algorithm>
#include "RubySlicc_ComponentMapping.h"
#include "NetworkMessage.h"
#include "Network.h"
#include <stdlib.h>
#include "RubyConfig.h"
#include "Consumer.h"
#include <vector>

void AMPMPrefetcher::observePfHit(const Address& address) { return;}
void AMPMPrefetcher::observePfMiss(const Address& address){ return;}
void AMPMPrefetcher::observeMiss(const Address& address, const AccessType& type) {return;}
void AMPMPrefetcher::observeAccess(const Address& address, const AccessType& type) {return;}


void AMPMPrefetcher::observePrefetchCallback(const Address& address)
{

    CacheAddr_t addr = address.getAddress();
    mshrpf->Done(addr);
}



void AMPMPrefetcher::issuePrefetch(COUNTER cycle)
{
    int ptr = mshrpf->ptr;
  // Quite ugly. Reaching into MSHR from here. Only because. MSHR does not have access to cache controller
  for(int i=0; i<PREF_MSHR_SIZE; i++) {
        int p = (ptr + i + 1) % PREF_MSHR_SIZE;
        if((mshrpf->mshr[p].Valid) && (!mshrpf->mshr[p].Issue))
        {
            // When the entry does not issued,
            // prefetch MSHR issues request to Memory Unit.
            mshrpf->mshr[p].IssuePrefetch(cycle);
            m_pref_ptr->enqueuePrefetchToCache(Address(mshrpf->mshr[p].Addr),AccessType_Read);
        }
    }
}



void AMPMPrefetcher::wakeup()
{

    COUNTER cycle = g_eventQueue_ptr->getTime();
    PrefetchData_t L2Data = m_pref_ptr->AccessData;

    mshrpf->Housekeeping(cycle);
    // asp->Housekeeping(cycle);

    /////////////////////////////////////////////////////
  /////////////////////////////////////////////////////
  // L2 Prefetching (AMPM Prefetching)
  //
  // Following part is pipelined Structure
  // Stage sequence is written in reverse order.
  // (This coding style might be similar to SimpleScalar)
  /////////////////////////////////////////////////////
  /////////////////////////////////////////////////////

  /////////////////////////////////////////////////////
  // Stage 3. Issue Prefetch from MSHR
  /////////////////////////////////////////////////////

    // [AS] This is where you check and issue.
 // mshrpf->PrefetchHousekeeping(mshr, cycle);
    issuePrefetch(cycle);
  /////////////////////////////////////////////////////
  // Stage 2. Issue Prefetch Request to MSHR
  /////////////////////////////////////////////////////

  // Maximum issue rate to prefetch MSHR...
  static const int MAX_ISSUE = 1;

  // This loop is implemented as a priority encoder
  for(int i=1, count=0;
      ( NumFwdPref && (count < MAX_ISSUE) &&
    (!mshrpf->Full()) && (i<(MAP_Size/2)) );
      i++) {
    if(FwdPrefMap[i]) {
      CacheAddr_t PrefAddress = AddressRegister + (i * BLK_SIZE);
      FwdPrefMap[i] = false;

      // Update Memory Access Map
      pref->UpdateEntry(cycle, PrefAddress);

      //if((GetPrefetchBit(1, PrefAddress) < 0) &&
      // !mshr->Search(PrefAddress)) {
        mshrpf->Issue  (cycle, PrefAddress);
      NumFwdPref--;
      count++;
    }
  }

  // This loop is implemented as a priority encoder
  for(int i=1, count=0;
      ( NumBwdPref && (count < MAX_ISSUE) &&
    (!mshrpf->Full()) && (i<(MAP_Size/2)) );
      i++) {
    if(BwdPrefMap[i] && (AddressRegister > (i * BLK_SIZE))) {
      CacheAddr_t PrefAddress = AddressRegister - (i * BLK_SIZE);
      BwdPrefMap[i] = false;

      // Update Memory Access Map
      pref->UpdateEntry(cycle, PrefAddress);

      // [AS] No need to filter prefetch. Cache will take care of this.
      //if((GetPrefetchBit(1, PrefAddress) < 0) &&
      // !mshr->Search(PrefAddress)) {
      // if(!mshr->Search(PrefAddress)) {

      mshrpf->Issue(cycle, PrefAddress);
      // }
      NumBwdPref--;
      count++;
    }
  }

  /////////////////////////////////////////////////////
  // Stage 1. Access to Memory Access Map Table
  /////////////////////////////////////////////////////

  // Memory Access Map Table Access
  if(L2Data.LastRequestCycle == cycle) {
    bool MSHR_hit =  mshrpf->Search(L2Data.DataAddr);

    // Update MSHR. [AS]. No need to update MSHR. and filter prefetches. Cache will take care.
    // This stored data is used for filtering prefetch requests...
    // if(!MSHR_hit && !L2Data.hit) {
      // mshr->Issue(cycle, L2Data.DataAddr);
    // }

    // Read prefetch candidate from Memory Access Map Table
    pref->IssuePrefetch(cycle, L2Data.DataAddr, MSHR_hit,
            FwdPrefMap, BwdPrefMap,
            &NumFwdPref, &NumBwdPref);
    AddressRegister = L2Data.DataAddr;
    AddressRegister &= ~(BLK_SIZE-1);
  }
}

AMPMPrefetcher::AMPMPrefetcher(AbstractChip* chip_ptr, GenericPrefetcher* g_pref)
{

    m_chip_ptr = chip_ptr;
    m_pref_ptr = g_pref;
    mshrpf = new PrefetchMissStatusHandlingRegister();
    pref = new MemoryAccessMapTable();
    NumFwdPref = 0;
    NumBwdPref = 0;
}

AMPMPrefetcher::~AMPMPrefetcher()
{

  delete mshrpf;
  delete pref;

}


void  AMPMPrefetcher::print() const
{

  std::cout << "Number of Fwd prefetches" << NumFwdPref;
  std::cout << "Number of Bwd prefetches" << NumBwdPref;

}




