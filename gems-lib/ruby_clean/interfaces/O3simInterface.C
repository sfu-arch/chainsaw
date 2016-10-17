#include "O3simInterface.h"

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "System.h"
#include "SubBlock.h"
#include "Chip.h"
#include "RubyConfig.h"
#include "Sequencer.h"

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

static CacheRequestType get_request_type( OpalMemop_t opaltype );
static OpalMemop_t get_opal_request_type( CacheRequestType type );

/// The static opalinterface instance
O3simInterface *O3simInterface::inst = NULL;

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
O3simInterface::O3simInterface(System* sys_ptr)
   : m_callback_queue(g_NUM_PROCESSORS)
{
  clearStats();
  ASSERT( inst == NULL );
  inst = this;
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
void O3simInterface::printConfig(ostream& out) const {
  out << "O3sim_ruby_multiplier: " << OPAL_RUBY_MULTIPLIER << endl;
  out << endl;
}

void O3simInterface::printStats(ostream& out) const {
  out << endl;
  out << "Gpusim Interface Stats" << endl;
  out << "----------------------" << endl;
  out << endl;
}

void O3simInterface::callbackQueuePrint(unsigned core_id) const
{
   assert(core_id < m_callback_queue.size());
   printf("CallbackQueue[%u]:", core_id);
   for (callback_queue::const_iterator iCallback = m_callback_queue[core_id].begin();
        iCallback != m_callback_queue[core_id].end(); ++iCallback)
   {
      printf("%llx ", *iCallback);
   }
   printf("\n");
}

//**************************************************************************
void O3simInterface::clearStats() {
}

static OpalMemop_t get_opal_request_type( CacheRequestType type ) {
  OpalMemop_t opal_type;

  if(type == CacheRequestType_LD){
    opal_type = OPAL_LOAD;
  }
  else if( type == CacheRequestType_ST){
    opal_type = OPAL_STORE;
  }
  else if( type == CacheRequestType_IFETCH){
    opal_type = OPAL_IFETCH;
  }
  else if( type == CacheRequestType_ATOMIC){
    opal_type = OPAL_ATOMIC;
  }
  else{
    ERROR_MSG("Error: Strange memory transaction type: not a LD or a ST");
  }

  //cout << "get_opal_request_type() CacheRequestType[ " << type << " ] opal_type[ " << opal_type << " ] " << endl;
  return opal_type;
}

static CacheRequestType get_request_type( OpalMemop_t opaltype ) {
  CacheRequestType type;

  if (opaltype == OPAL_LOAD) {
    type = CacheRequestType_LD;
  } else if (opaltype == OPAL_STORE){
    type = CacheRequestType_ST;
  } else if (opaltype == OPAL_IFETCH){
    type = CacheRequestType_IFETCH;
  } else if (opaltype == OPAL_ATOMIC){
    type = CacheRequestType_ATOMIC;
  } else {
    ERROR_MSG("Error: Strange memory transaction type: not a LD or a ST");
  }
  return type;
}



//**************************************************************************

void O3simInterface::advanceTime( void ) {

  g_eventQueue_ptr->triggerEvents(g_eventQueue_ptr->getTime()+1);
}

// return ruby's time
//**************************************************************************
unsigned long long O3simInterface::getTime( void ) {
  return (g_eventQueue_ptr->getTime());
}

// print's Ruby outstanding request table
void O3simInterface::printProgress(int cpuNumber){
  Sequencer* targetSequencer_ptr = g_system_ptr->getChip(cpuNumber/RubyConfig::numberOfProcsPerChip())->getSequencer(cpuNumber%RubyConfig::numberOfProcsPerChip());
  assert(targetSequencer_ptr != NULL);

  targetSequencer_ptr->printProgress(cout);
}


void  O3simInterface::notify( int status ) {
  if (O3simInterface::inst == NULL) {
    if (status == 1) {
      // notify system that opal is now loaded
      g_system_ptr->opalLoadNotify();
    } else {
      return;
    }
  }

  // opal interface must be allocated now
  ASSERT( O3simInterface::inst != NULL );
}


void  O3simInterface::makeRequest( unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid, bool is_write, bool isPriv, class mem_fetch * mf )
{
  unsigned cpuNumber = sid;
  unsigned long long physicalAddr = addr;
  unsigned long long logicalAddr = addr;
  unsigned long long virtualPC = 0;
  unsigned thread = tid;
  unsigned requestSize = req_size;
  CacheRequestType req_type = CacheRequestType_NULL;

  AccessModeType access_mode;
  if (isPriv) {
    access_mode = AccessModeType_SupervisorMode;
  } else {
    access_mode = AccessModeType_UserMode;
  }

  req_type = is_write ? CacheRequestType_ST : CacheRequestType_LD;

  // Send request to sequencer
  Sequencer* targetSequencer_ptr = g_system_ptr->getChip(cpuNumber/RubyConfig::numberOfProcsPerChip())->getSequencer(cpuNumber%RubyConfig::numberOfProcsPerChip());

  targetSequencer_ptr->makeRequest(CacheMsg(Address( physicalAddr ),
                                            Address( physicalAddr ),
                                            req_type,
                                            Address(virtualPC),
                                            access_mode,   // User/supervisor mode
                                            requestSize,   // Size in bytes of request
                                            PrefetchBit_No, // Not a prefetch
                                            0,              // Version number
                                            Address(logicalAddr),   // Virtual Address
                                            thread,              // SMT thread
                                            0,          // TM specific - timestamp of memory request
                                            false      // TM specific - whether request is part of escape action
                                            )
                                   );
}


//**************************************************************************
void  O3simInterface::makePrefetch(int  cpuNumber, la_t logicalAddr, pa_t physicalAddr,
                                  int  requestSize, OpalMemop_t typeOfRequest,
                                  la_t virtualPC, int isPriv, int thread) {
  DEBUG_MSG(SEQUENCER_COMP,MedPrio,"Making prefetch request");

  // Issue the request to the sequencer.
  // set access type (user/supervisor)
  AccessModeType access_mode;
  if (isPriv) {
    access_mode = AccessModeType_SupervisorMode;
  } else {
    access_mode = AccessModeType_UserMode;
  }

  // make the prefetch
  Sequencer* targetSequencer_ptr = g_system_ptr->getChip(cpuNumber/RubyConfig::numberOfProcsPerChip())->getSequencer(cpuNumber%RubyConfig::numberOfProcsPerChip());
  targetSequencer_ptr->makeRequest(CacheMsg(Address( physicalAddr ),
                                            Address( physicalAddr ),
                                            get_request_type(typeOfRequest),
                                            Address(virtualPC),
                                            access_mode,
                                            requestSize,
                                            PrefetchBit_Yes, // True means this is a prefetch
                                            0,              // Version number
                                            Address(logicalAddr),   // Virtual Address
                                            thread,              // SMT thread
                                            0,          // TM specific - timestamp of memory request
                                            false      // TM specific - whether request is part of escape action
                                            )
                                   );
  return;
}


//**************************************************************************
bool  O3simInterface::isReady( unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid, bool is_write, class mem_fetch * mf )
{
  unsigned cpuNumber = sid;
  unsigned long long physicalAddr = addr;
  unsigned long long logicalAddr = addr;
  unsigned long long virtualPC = 0;
  unsigned thread = tid;
  CacheRequestType req_type = is_write? CacheRequestType_ST : CacheRequestType_LD;
  unsigned requestSize = req_size;

  // Send request to sequencer
  Sequencer* targetSequencer_ptr = g_system_ptr->getChip(cpuNumber/RubyConfig::numberOfProcsPerChip())->getSequencer(cpuNumber%RubyConfig::numberOfProcsPerChip());

  // FIXME - some of these fields have bogus values sinced isReady()
  // doesn't need them.  However, it would be cleaner if all of these
  // fields were exact.

  // if (cpuNumber == 0)
  //   printf("Ruby::isReady(proc=%d,addr=%llx,tid=%d)\n", cpuNumber, addr, thread);
  return (targetSequencer_ptr->isReady(CacheMsg(Address( physicalAddr ),
                                                Address( physicalAddr ),
                                                req_type,
                                                Address(0),
                                                AccessModeType_UserMode,   // User/supervisor mode
                                                requestSize,   // Size in bytes of request
                                                PrefetchBit_No,  // Not a prefetch
                                                0,              // Version number
                                                Address(logicalAddr),   // Virtual Address
                                                thread,              // SMT thread
                                                0,          // TM specific - timestamp of memory request
                                                false      // TM specific - whether request is part of escape action
                                                )
                                       ));
}


bool O3simInterface::tryAccCacheAccess(const unsigned long long addr)
{
  unsigned cpuNumber = RubyConfig::numberOfProcessors() - 1;
  Sequencer* targetSequencer_ptr = g_system_ptr->getChip(cpuNumber/RubyConfig::numberOfProcsPerChip())->getSequencer(cpuNumber%RubyConfig::numberOfProcsPerChip());

  return targetSequencer_ptr->tryAccCacheAccess(Address(addr));
}



bool O3simInterface::tryL1CacheAccess(const unsigned long long addr)
{
  unsigned cpuNumber = RubyConfig::numberOfProcessors() - 1;
  Sequencer* targetSequencer_ptr = g_system_ptr->getChip(cpuNumber/RubyConfig::numberOfProcsPerChip())->getSequencer(cpuNumber%RubyConfig::numberOfProcsPerChip());

  return targetSequencer_ptr->tryL1CacheAccess(Address(addr));
}


//
// Change to assertions
//
// THIS IS CALLBACK TO GPGPUSIM
// ADD TO QUEUE during CALLBACK
void O3simInterface::hitCallback(NodeID proc, SubBlock& data, CacheRequestType type, int thread) {
  // notify opal that the transaction has completed
  //(*m_opal_intf->hitCallback)( proc, data.getAddress().getAddress(), get_opal_request_type(type), thread );
  unsigned long long addr = data.getAddress().getAddress();
  // if (addr == WATCH_ADDR)
  //   printf("Ruby::hitCallback(proc=%d,addr=%llx,tid=%d)\n", proc, addr, thread);
  m_callback_queue[proc].push_back(addr);
}

//**************************************************************************
// Useful functions if Ruby needs to read/write physical memory when running with Opal
integer_t O3simInterface::readPhysicalMemory(int procID,
                                           physical_address_t address,
                                           int len ){
    assert(0);
    return 0;
  //return SIMICS_read_physical_memory(procID, address, len);
}

//**************************************************************************
void O3simInterface::writePhysicalMemory( int procID,
                                        physical_address_t address,
                                        integer_t value,
                                        int len ){
    assert(0);
  //SIMICS_write_physical_memory(procID, address, value, len);
}

//***************************************************************
// notifies Opal to print debug info
void O3simInterface::printDebug(){
  //(*m_opal_intf->printDebug)();
}


