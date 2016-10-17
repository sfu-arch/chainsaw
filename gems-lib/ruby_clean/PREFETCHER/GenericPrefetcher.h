#ifndef GENERIC_PREFETCHER_H
#define GENERIC_PREFETCHER_H 

#include "Global.h"
#include "RubyConfig.h"
#include "Address.h"
#include "Consumer.h"
#include "Types.h"
#include "Chip.h"
#include "AbstractChip.h"
#include "AbstractPrefetcher.h"
#include "SubBlock.h"
#include "MSHR.h"
#include "pref_types.h"


class AbstractChip;
class AbstractPrefetcher;

typedef struct
{
  COUNTER      LastRequestCycle;    // Cycle in which this request took place
  CacheAddr_t  LastRequestAddr;     // Program Counter of the instruction that made the request 
  unsigned long  SequenceNumber;      // Sequence number of the instruction that made the request
  AccessType       m_accesstype;          // Request type (Load:1, Store:2)
  CacheAddr_t  DataAddr;            // Virtual address for the data being requested
  bool         hit;                 // Did the request hit in the cache?
  bool         LastRequestPrefetch; // Was this a prefetch request?
} PrefetchData_t;

class GenericPrefetcher : public Consumer {
public:

  // Constructors
  GenericPrefetcher(AbstractChip* chip_ptr, string config,int version,MachineType m_type);

~GenericPrefetcher();
  /**
   * Implement the prefetch hit(miss) callback interface.
   * These functions are called by the cache when it hits(misses)
   * on a line with the line's prefetch bit set. If this address
   * hits in m_array we will continue prefetching the stream
   * 
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
    

    void observePrefetchCallback(const Address& address);

    /**
    * Print out some statistics
    */
    void print(std::ostream& ) const;

    /**
     * Some prefetchers would like wakeup every cycle. Other prefetchers don't care.
     * In general we want to push into a queue which is cleared every cycle. 
     */
    void wakeup();

    void enqueuePrefetchToCache (Address line_addr, AccessType type);
   
	/**
 * 	 * Information about latest access.
	 */
	PrefetchData_t AccessData;

	/**
	 * Information about latest miss.
	 */
	PrefetchData_t MissData;

  static void printConfig(ostream& out);


private:
	  // data members
	AbstractChip* m_chip_ptr;
  AbstractPrefetcher* m_pref;

  string m_pref_name;
     // Consumer to signal a wakeup()
  MachineType m_machType;
  int m_version;
  int misses;

};


// Output operator declaration
ostream& operator<<(ostream& out, const GenericPrefetcher& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const GenericPrefetcher& obj)
{
  obj.print(out);
  out << flush;
  return out;
}


#endif
