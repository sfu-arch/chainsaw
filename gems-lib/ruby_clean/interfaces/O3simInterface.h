#ifndef O3SIM_INTERFACE_H
#define O3SIM_INTERFACE_H

#include "RubyConfig.h"
#include "Global.h"
#include "Driver.h"
#include "mf_api.h"
#include "CacheRequestType.h"

#include <vector>
#include <list>

class System;
class Chip;
class Sequencer;

class O3simInterface : public Driver {
public:
  // Constructors
    O3simInterface(System* sys_ptr);

  // Destructor
  // ~OpalInterface();

  void hitCallback( NodeID proc, SubBlock& data, CacheRequestType type, int thread );
  void printStats(ostream& out) const;
  void clearStats();
  void printConfig(ostream& out) const;

  integer_t readPhysicalMemory(int procID, physical_address_t address,
                               int len );

  void writePhysicalMemory( int procID, physical_address_t address,
                            integer_t value, int len );


  void printDebug();

  /// The static O3simInterface instance
  static O3simInterface *inst;

  /// static methods

  /* returns true if the sequencer is able to handle more requests.
     This implements "back-pressure" by which the processor knows
     not to issue more requests if the network or cache's limits are reached.
   */
  static bool isReady( unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid,
                      bool is_write, class mem_fetch * mf );

  /*
  makeRequest performs the coherence transactions necessary to get the
  physical address in the cache with the correct permissions. More than
  one request can be outstanding to ruby, but only one per block address.
  The size of the cache line is defined to Intf_CacheLineSize.
  When a request is finished (e.g. the cache contains physical address),
  ruby calls completedRequest(). No request can be bigger than
  Opal_CacheLineSize. It is illegal to request non-aligned memory
  locations. A request of size 2 must be at an even byte, a size 4 must
  be at a byte address half-word aligned, etc. Requests also can't cross a
  cache-line boundaries.
  */
  static void makeRequest( unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid,
                           bool is_write, bool isPriv, class mem_fetch * mf );

  /* prefetch a given block...
   */
  static void makePrefetch(int cpuNumber, la_t logicalAddr, pa_t physicalAddr,
                           int requestSize, OpalMemop_t typeOfRequest,
                           la_t virtualPC, int isPriv, int thread);


  static bool tryAccCacheAccess(const unsigned long long addr);

  static bool tryL1CacheAccess(const unsigned long long addr);

  /* notify ruby of opal's status
   */
  static void notify( int status );

  /*
   * advance ruby one cycle
   */
  static void advanceTime( void );

  /*
   * return ruby's cycle count.
   */
  static unsigned long long getTime( void );

  /* prints Ruby's outstanding request table */
  static void printProgress(int cpuNumber);

  // interaction with the callback queue - empty(), top(), pop()
  bool RubyQueueEmpty(unsigned core_id) {
    assert(core_id < m_callback_queue.size());
    return m_callback_queue[core_id].empty();
  }
  unsigned long long RubyQueueTop(unsigned core_id) {
    assert(core_id < m_callback_queue.size());
    return m_callback_queue[core_id].front();
  }
  void RubyQueuePop(unsigned core_id) {
    assert(core_id < m_callback_queue.size());
    return m_callback_queue[core_id].pop_front();
  }
  void callbackQueuePrint(unsigned core_id) const;



private:
  // Private Methods


  // Data Members (m_ prefix)

  // queue storing the address assocated with each callback
  typedef std::list<unsigned long long> callback_queue;
  std::vector<callback_queue> m_callback_queue;

};

#endif // GPUSIM_INTERFACE_H

