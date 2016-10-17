#ifndef AMPMPREFETCHER_H
#define AMPMPREFETCHER_H

#include <bitset>
#include "MessageBuffer.h"
#include "AbstractChip.h"
#include "Global.h"
#include "AccessPermission.h"
#include "Address.h"
#include "CacheRecorder.h"
 #include "MessageBuffer.h"
#include "Vector.h"
#include "Types.h"
#include "DataBlock.h"
#include "Protocol.h"
#include "MachineType.h"
#include "RubySlicc_ComponentMapping.h"
#include "pref_types.h"
#include "MSHR.h"


class GenericPrefetcher;


////////////////////////////////////////////////////
// Definition of Constants.
////////////////////////////////////////////////////

// Access Map Size
static const int MAP_Size  = 1 << IDX_WIDTH;

// Memory Access Map Table Size
// #define REALISTIC
// Our submission is realistic configuration

#ifdef REALISTIC
// Realistic Configuration
// 13 way set associative table
static const int NUM_SET =  4;
static const int NUM_WAY = 13;

#else
// Idealistic Configuration
// Full associative table
static const int NUM_SET =  1;
static const int NUM_WAY = 52;

#endif

// TABLE SIZE is 52 entries because of the storage limitation.
// If  we have more budget, we will emply 64 entries.
// And we have less budget, we will emply 32 entries.
static const int TABLE_SIZE = NUM_SET * NUM_WAY;

// Threshold
// They are only the constansts.
static const int AP_THRESHOLD = 8;
static const int SE_THRESHOLD = 128;
static const int CA_THRESHOLD = 256;

////////////////////////////////////////////////////
// Definition of Constants.
////////////////////////////////////////////////////

// Memory access map status is implemented as 2bit state machine
enum MemoryAccessMapState {
  INIT    , // INIT     Status (00)
  ACCESS  , // ACCESS   Status (01)
  PREFETCH, // PREFETCH Status (10)
  SUCCESS , // SUCCESS  Status (11)
};

// Generate Index and Tag
static CacheAddr_t GenTag(CacheAddr_t addr){ return (addr >> IDX_BITS) & TAG_MASK; }
static CacheAddr_t GenIdx(CacheAddr_t addr){ return (addr >> BLK_BITS) & IDX_MASK; }

class MemoryAccessMap {
public:
  int LRU; // 6 bits LRU information

  COUNTER LastAccess; // Timer(16 bits)
  COUNTER NumAccess;  // Access counter(4 bits)
  CacheAddr_t Tag;    // 18 bit address tag(18 bits)

  // 2 bit state machine x 256 = 512 bit Map
  enum MemoryAccessMapState AccessMap[MAP_Size];

  // Total Budget Count : 556 bits

public:
  /////////////////////////////////////////////////////
  // Constructer & Copy Constructer
  /////////////////////////////////////////////////////
  MemoryAccessMap(CacheAddr_t t=0) : Tag(t) { }
  MemoryAccessMap(const MemoryAccessMap& ent) : Tag(ent.Tag) { }
  ~MemoryAccessMap(){}

  /////////////////////////////////////////////////////
  // Read prefetch infomation
  /////////////////////////////////////////////////////
  bool Hit(CacheAddr_t addr) {
    return Tag == GenTag(addr);
  }

  bool AlreadyAccess(CacheAddr_t addr) {
    return (Hit(addr) && (AccessMap[GenIdx(addr)] != INIT));
  }

  bool PrefetchHit(CacheAddr_t addr) {
    return (Hit(addr) && (AccessMap[GenIdx(addr)] == INIT));
  }

  void Read(enum MemoryAccessMapState *ReadMap) {
    for(int i=0; i<MAP_Size; i++){
      ReadMap[i] = AccessMap[i];
    }
  }

  /////////////////////////////////////////////////////
  // Updating methods...
  /////////////////////////////////////////////////////

  // Assign new Czone to this map.
  void Entry(CacheAddr_t addr){
    // Assign & Initializing Entry
    Tag = GenTag(addr);
    for(int i=0; i<MAP_Size; i++) { AccessMap[i] = INIT; }
    LastAccess = 0;
    NumAccess = 0;
  }

  // Issuing prefetch request
  bool IssuePrefetchSub(COUNTER cycle, CacheAddr_t addr){
    if(!Hit(addr)) return false;

    // L2 Access Control
    ASSERT(AccessMap[GenIdx(addr)] == INIT);
    AccessMap[GenIdx(addr)]=PREFETCH;
    return true;
  }

  // Updating state machine
  void Update(COUNTER cycle, CacheAddr_t addr){
    ASSERT(Hit(addr));

    switch(AccessMap[GenIdx(addr)]) {
    case INIT    : AccessMap[GenIdx(addr)] = ACCESS; break;
    case ACCESS  : return;
    case PREFETCH: AccessMap[GenIdx(addr)] = SUCCESS; break;
    case SUCCESS : return;
    default    : ASSERT(false); break;
    }

    if(NumAccess >= 15) {
      NumAccess  = 8;
      LastAccess = LastAccess >> 1;
    } else {
      NumAccess++;
    }

    return;
  }

  /////////////////////////////////////////////////////
  // Profiling methods
  /////////////////////////////////////////////////////
  int NumInit() {
    int Succ = 0;
    for(int i=0; i<MAP_Size; i++) {
      Succ += (AccessMap[i] == INIT ? 1 : 0);
    }
    return Succ;
  }
  int NumSuccPref() {
    int Succ = 0;
    for(int i=0; i<MAP_Size; i++) {
      Succ += (AccessMap[i] == SUCCESS ? 1 : 0);
    }
    return Succ;
  }
  int NumAccesses() {
    int Succ = 0;
    for(int i=0; i<MAP_Size; i++) {
      Succ += (AccessMap[i] == ACCESS ? 1 : 0);
    }
    return Succ;
  }
  int NumFailPref() {
    int Succ = 0;
    for(int i=0; i<MAP_Size; i++) {
      Succ += (AccessMap[i] == PREFETCH ? 1 : 0);
    }
    return Succ;
  }

  // Returns stream length of the prefetch 
  // * The stream length is decided from access history
  int MaxAccess() {
    int req = (NumAccess * 256) / (1 + LastAccess);
    int max = 7;
    return max > req ? req : max;
  }

  /////////////////////////////////////////////////////
  // Housekeeping
  /////////////////////////////////////////////////////

  // Updates some counters.
  void Housekeeping(COUNTER cycle){
    LastAccess++;
    if(LastAccess > 65536) {
      LastAccess = 65536;
    }
  }
};

class MemoryAccessMapTable {
public:
  // These flags uses 3 bits
  bool Aggressive;
  bool SaveEntry;
  bool ConflictAvoid;

  // These counter uses 128 bits
  int  NeedEntry;
  int  Conflict;
  int  PrefFail;
  int  PrefSucc;

  // 54 entries table (556 x 52)
  MemoryAccessMap Table[NUM_SET][NUM_WAY];

  // Total Budget Count = 29147 bits

  /////////////////////////////////////////////
  // Constructor & Copy Constructor
  /////////////////////////////////////////////

  MemoryAccessMapTable() {
    NeedEntry     = 64;
    Conflict      = 16;
    SaveEntry     = false;
    ConflictAvoid = false;
    Aggressive    = false;

    for(int Idx=0; Idx<NUM_SET; Idx++) {
      for(int Way=0; Way<NUM_WAY; Way++) {
	Table[Idx][Way].LRU = Way;
	Table[Idx][Way].Entry(0);
      }
    }
  }

  ~MemoryAccessMapTable() { }
  
  /////////////////////////////////////////////
  // Basic Functions.
  /////////////////////////////////////////////

  // Status Handling Function
  bool IsAccessed(MemoryAccessMapState state) {
    switch(state) {
    case INIT    : return false;
    case ACCESS  : return true;
    case PREFETCH: return Aggressive;
    case SUCCESS : return true;
    default    : ASSERT(false); return false;
    }
  }
  
  bool IsCandidate(MemoryAccessMapState state, int index, int passivemode) {
    return ((state == INIT) &&
	    ((index >= MAP_Size && index < (2*MAP_Size)) || !passivemode));
  }

  /////////////////////////////////////////////
  // Read & Update memory access map
  /////////////////////////////////////////////

  // Read Memory Access Map from Table
  MemoryAccessMap* AccessEntry(COUNTER cycle, CacheAddr_t addr) {
    int Oldest = 0;

    int Idx = GenTag(addr) % NUM_SET;
    ASSERT(Idx >= 0 && Idx < NUM_SET);
    for(int Way=0; Way<NUM_WAY; Way++) {
      if(Table[Idx][Way].Hit(addr)) {
	// Find Entry
	for(int i=0; i<NUM_WAY; i++) {
	  if(Table[Idx][i].LRU < Table[Idx][Way].LRU) {
	    Table[Idx][i].LRU++;
	    ASSERT(Table[Idx][i].LRU < NUM_WAY);
	  }
	}
	Table[Idx][Way].LRU = 0;
	return (&(Table[Idx][Way]));
      }

      if(Table[Idx][Way].LRU > Table[Idx][Oldest].LRU) {
	Oldest = Way;
      }
    }

    ASSERT(Table[Idx][Oldest].LRU == (NUM_WAY-1));

    // Find Entry
    for(int i=0; i<NUM_WAY; i++) {
      if(Table[Idx][i].LRU < Table[Idx][Oldest].LRU) {
	Table[Idx][i].LRU++;
	ASSERT(Table[Idx][i].LRU < NUM_WAY);
      }
    }
    PrefFail += Table[Idx][Oldest].NumFailPref();
    PrefSucc += Table[Idx][Oldest].NumSuccPref();
    Table[Idx][Oldest].Entry(addr);
    
    Table[Idx][Oldest].LRU = 0;
    return (&(Table[Idx][Oldest]));
  }

  // Update Memory Access Map when the entry is on the table.
  void UpdateEntry(COUNTER cycle, CacheAddr_t addr) {
    // Issueing Prefetch
    int Idx = GenTag(addr) % NUM_SET;
    ASSERT(Idx >= 0 && Idx < NUM_SET);
    for(int Way=0; Way<NUM_WAY; Way++) {
      if(Table[Idx][Way].Hit(addr)) {
	Table[Idx][Way].IssuePrefetchSub(cycle, addr);
	return;
      }
    }
  }
  
  ///////////////////////////////////////////////////
  // Read access map & generate prefetch candidates.
  ///////////////////////////////////////////////////

  // Read Prefetch Candidates from Memory Access Map
  // This function does not has any memories.
  void IssuePrefetch(COUNTER cycle, CacheAddr_t addr, bool MSHR_hit,
		     bool *FwdPrefMap, bool *BwdPrefMap,
		     int  *NumFwdPref, int *NumBwdPref) {
    //////////////////////////////////////////////////////////////////
    // Definition of Temporary Values
    //////////////////////////////////////////////////////////////////

    // These variables are not counted as a storage
    // Since these are used as a wire (not register)
    MemoryAccessMap *ent_l, *ent_m, *ent_h;
    static enum MemoryAccessMapState ReadMap[MAP_Size * 3];
    static enum MemoryAccessMapState FwdHistMap[MAP_Size];
    static enum MemoryAccessMapState BwdHistMap[MAP_Size];
    static bool FwdCandMap[MAP_Size];
    static bool BwdCandMap[MAP_Size];
    static bool FwdPrefMapTmp[MAP_Size/2];
    static bool BwdPrefMapTmp[MAP_Size/2];
    static int  NumFwdPrefTmp;
    static int  NumBwdPrefTmp;

    bool PassiveMode = SaveEntry || ConflictAvoid; // Mode
     
    //////////////////////////////////////////////////////////////////
    // Read & LRU update for access table
    //////////////////////////////////////////////////////////////////
    if(PassiveMode) {
      ent_l = NULL;
      ent_h = NULL;
    } else {
      ent_l = AccessEntry(cycle, addr - IDX_SIZE);
      ent_h = AccessEntry(cycle, addr + IDX_SIZE);
    }
    ent_m = AccessEntry(cycle, addr);

    //////////////////////////////////////////////////////////////////    
    // Following Cases are not prefered.
    //
    // 1. AccessMap==0 && PrefetchBit==1 (Prefetched)
    //      -> There are not enough Entry Size
    // 2. L2 Miss && Already Accessed && MSHR Miss (Finished filling)
    //      -> L2 Cache conflict miss is detected.
    //
    // Our prefetcher detect these situation and profiling these case.
    // When the frequenty exceed some threshold,
    // the profiler changes the prefetch mode.
    //////////////////////////////////////////////////////////////////
    //
    //
    // [AS] I am just going to set the prefetch automatically.
    bool PF_succ = false; // Prefetch successed
    bool L2_miss = true; // L2 miss detection
//    [AS] I don't think this is needed
/*    UnSetPrefetchBit(1, addr); // Clear Prefetch Bit
  */  
    bool NE_INC = PF_succ && ent_m->PrefetchHit(addr);
    bool CF_INC = L2_miss && ent_m->AlreadyAccess(addr) && !MSHR_hit;
    NeedEntry += NE_INC ? 1 : 0;
    Conflict  += CF_INC ? 1 : 0;


    //////////////////////////////////////////////////////////////////
    // Prefetching Part...
    //////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////
    // Read Entry (Read & Concatenate Accessed Maps)
    //////////////////////////////////////////////////////////////////
    for(int i=0; i<MAP_Size; i++) { ReadMap[i] = INIT; }

    if(ent_l) ent_l->Read(&ReadMap[0 * MAP_Size]);
    if(ent_m) ent_m->Read(&ReadMap[1 * MAP_Size]);
    if(ent_h) ent_h->Read(&ReadMap[2 * MAP_Size]);

    //////////////////////////////////////////////////////////////////
    // Generating Forwarding Prefetch
    //////////////////////////////////////////////////////////////////
    if(ConflictAvoid) {
      NumFwdPrefTmp = 2;
    } else {
      NumFwdPrefTmp = 2
	+ (ent_m ? ent_m->MaxAccess() : 0);
      + (PassiveMode && ent_l ? 0 : ent_l->MaxAccess());
    }
    
    // This block should be implemented in simple shifter
    for(int i=0; i<MAP_Size; i++) {
      int index_p = MAP_Size + GenIdx(addr) + i;
      int index_n = MAP_Size + GenIdx(addr) - i;
      FwdHistMap[i] = ReadMap[index_n];
      FwdCandMap[i] = IsCandidate(ReadMap[index_p], index_p, PassiveMode);
    }
    
    // This for statement can be done in parallel
    for(int i=0; i<MAP_Size/2; i++) {
      FwdPrefMapTmp[i] =
	FwdCandMap[i] &&
	IsAccessed(FwdHistMap[i]) &&
	( IsAccessed(FwdHistMap[2*i+0]) ||
	  IsAccessed(FwdHistMap[2*i+1]) );
    }

    //////////////////////////////////////////////////////////////////
    // Generating Backward Prefetch
    //////////////////////////////////////////////////////////////////

    if(ConflictAvoid) {
      NumBwdPrefTmp = 2;
    } else {
      NumBwdPrefTmp = 2
	+ (ent_m ? ent_m->MaxAccess() : 0);
	+ (PassiveMode && ent_h ? 0 : ent_h->MaxAccess());
    }

    for(int i=0; i<MAP_Size; i++) {
      int index_p = MAP_Size + GenIdx(addr) + i;
      int index_n = MAP_Size + GenIdx(addr) - i;
      BwdHistMap[i] = ReadMap[index_p];
      BwdCandMap[i] = IsCandidate(ReadMap[index_n], index_n, PassiveMode);
    }
    
    // This for statement can be done in parallel
    for(int i=0; i<MAP_Size/2; i++) {
      BwdPrefMapTmp[i] =
	BwdCandMap[i] &&
	IsAccessed(BwdHistMap[i]) &&
	( IsAccessed(BwdHistMap[2*i+0]) ||
	  IsAccessed(BwdHistMap[2*i+1]) );
    }

    //////////////////////////////////////////////////////////////////
    // Update Entry
    //////////////////////////////////////////////////////////////////

    ent_m->Update(cycle, addr);

    //////////////////////////////////////////////////////////////////
    // Setting Output Values (Copy to Arguments)
    //////////////////////////////////////////////////////////////////

    *NumFwdPref = NumFwdPrefTmp;
    *NumBwdPref = NumBwdPrefTmp;
    for(int i=0; i<MAP_Size/2; i++) {
      BwdPrefMap[i] = BwdPrefMapTmp[i];
      FwdPrefMap[i] = FwdPrefMapTmp[i];
    }
  }


  ///////////////////////////////////////////////////
  // Housekeeping Function
  ///////////////////////////////////////////////////

  void Housekeeping(COUNTER cycle){
    // Housekeeping for each entry.
    for(int Idx=0; Idx<NUM_SET; Idx++) {
      for(int Way=0; Way<NUM_WAY; Way++) {
	Table[Idx][Way].Housekeeping(cycle);
      }
    }

    // Aggressive Prefetching Mode
    // This mode is enabled when the 
    // (prefetch success count > 8 * prefetch fail count) is detected.
    // If this mode is enabled,
    // the prefetcher assume PREFETCH state as the ACCESS state.
    if((cycle & (((COUNTER)1 << 18)-1)) == 0) {
      if(PrefSucc > AP_THRESHOLD * PrefFail) {
	// printf("Aggressive Prefetch Mode\n");
	Aggressive = true;
      } else if(PrefSucc < (AP_THRESHOLD/2) * PrefFail) {
	// printf("Normal Prefetch Mode\n");
	Aggressive = false;
      }
      PrefSucc >>= 1;
      PrefFail >>= 1;
    }

    // Profiling Execution Status
    // This mode is enabled when the replacement of an access table entry
    // occurs frequently. When this mode is enabled,
    // the prefetcher stops reading neighbor entries.
    // This feature reduces the unprefered access entry replacements.
    if((cycle & (((COUNTER)1 << 18)-1)) == 0) {
      if(NeedEntry > SE_THRESHOLD) {
	// printf("Save Entry Mode\n");
	SaveEntry = true;
      } else if(NeedEntry > (SE_THRESHOLD/4)) {
	// printf("Normal Entry Mode\n");
	SaveEntry = false;
      }
      NeedEntry >>= 1;
    }

    // Profiling Execution Status
    // This mode is enabled when the L2 conflict misses are detected frequentry.
    // When the conflict avoidance mode is enabled,
    // the prefetcher reduces the number of prefetch requests.
    if((cycle & (((COUNTER)1 << 18)-1)) == 0) {
      if(Conflict > CA_THRESHOLD) {
	// printf("Conflict Avoidance Mode\n");
	ConflictAvoid = true;
      } else if(Conflict > (CA_THRESHOLD/4)) {
	// printf("Normal Conflict Mode\n");
	ConflictAvoid = false;
      }
      Conflict >>= 1;
    }
  }
};




class AMPMPrefetcher : public AbstractPrefetcher
{
public:
	 AMPMPrefetcher(AbstractChip*,GenericPrefetcher* g_pref_ptr);
	~AMPMPrefetcher();


        /**
         * Implement the prefetch hit(miss) callback interface.
         * These functions are called by the cache when it hits(misses)
         * on a line with the line's prefetch bit set. If this address
         * hits in m_array we will continue prefetching the stream.
         */
    void observePfHit(const Address& address);
    void observePfMiss(const Address& address);

        /**
         * Observe a memory miss from the cache.
         *
         * @param address   The physical address that missed out of the cache.
         */
    void observeMiss(const Address& address, const AccessType& type);


    void observeAccess(const Address& address, const AccessType& type);

    void issuePrefetch(COUNTER cycle);

       /**
        * Protocol calls in to inform that prefetch has finished
        * @param address [description]
        */
    void observePrefetchCallback(const Address& address);


    void wakeup();
        /**
         * Print out some statistics
         */
   void print() const;
       
private: 

    AbstractChip* m_chip_ptr;

    MachineType m_machType;

    GenericPrefetcher* m_pref_ptr;

    PrefetchMissStatusHandlingRegister *mshrpf;

    // Memory Access Map Table(29147 bits)
    MemoryAccessMapTable *pref;


    int NumFwdPref;              // 5 bit counter
	int NumBwdPref;              // 5 bit counter
	bool FwdPrefMap[MAP_Size/2]; // 128 bit register
	bool BwdPrefMap[MAP_Size/2]; // 128 bit register
	CacheAddr_t AddressRegister; // 26 bit register


	/* data */
};









#endif