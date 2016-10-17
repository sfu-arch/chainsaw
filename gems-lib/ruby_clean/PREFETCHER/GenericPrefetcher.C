#include "Global.h"
#include "AbstractChip.h"
#include "RubyConfig.h"
#include "Address.h"
#include "Consumer.h"
#include "AbstractPrefetcher.h"
#include "System.h"
#include "SubBlock.h"
#include "Chip.h"
#include "GenericPrefetcher.h"
#include "Prefetcher.h"
#include "AMPMPrefetcher.h"
#include <assert.h>




GenericPrefetcher::GenericPrefetcher(AbstractChip* chip_ptr, string config, int version,MachineType machType)
{

    m_chip_ptr = chip_ptr;

    if (config == "StridedPrefetcher")
    {
        m_pref = new StridePrefetcher(chip_ptr,this);
        m_pref_name = config;
    }
    else if(config == "AMPMPrefetcher")
    {
        m_pref = new AMPMPrefetcher(chip_ptr,this);
        m_pref_name = config;
    }
    else
        assert(0);

    m_machType = machType;
    m_version = version;
    g_eventQueue_ptr->scheduleEvent(this, 2);
}


GenericPrefetcher::~GenericPrefetcher()
{
//    print();
    delete m_pref;
}


void GenericPrefetcher::print (ostream& out) const {

/**
 *
 * Call print stats for prefetcher
 */
 m_pref->print();
}


void GenericPrefetcher::printConfig(ostream& out)
{
  out << "The Prefetcher Allocated is " << endl;
}

void GenericPrefetcher::enqueuePrefetchToCache(Address physicalAddr, AccessType type)
{

CacheRequestType ctype;

// Convert access type into specific type.
if (type == AccessType_Read)
    ctype = CacheRequestType_LD;
else
    ctype = CacheRequestType_ST;


// Create cache message type to push into prefetch queue.
physicalAddr.makeLineAddress();
CacheMsg* msg = new CacheMsg(physicalAddr,
                                         physicalAddr,
                                            ctype ,
                                            Address(0),
                                            AccessModeType_UserMode,   // User/supervisor mode
                                            8,   // Size in bytes of request
                                            PrefetchBit_No, // Not a prefetch
                                            0,              // Version number
                                            Address(0),   // Virtual Address
                                            0,              // SMT thread
                                            0,          // TM specific - timestamp of memory request
                                            false      // TM specific - whether request is part of escape action
                                            );

/*Control which queue the request gets into.
 Can push into mandatory but since L2 doesn't have mandatory to keep things symmetric we put an optional queue for both L1 and L2/
*/

if (m_machType == MACHINETYPE_L1CACHE_ENUM) {
    m_chip_ptr->m_L1Cache_optionalQueue_vec[m_version]->enqueue(*msg, 2);
    }
else if (m_machType == MACHINETYPE_L2CACHE_ENUM) {
    m_chip_ptr->m_L2Cache_optionalQueue_vec[m_version]->enqueue(*msg, 2);
    }
else {
    ASSERT(0);
    }
}


void GenericPrefetcher::observePfHit(const Address& address)
{
    if (!misses) return;

    m_pref->observePfHit(address);
}

void GenericPrefetcher::observePfMiss(const Address& address)
{

    if (!misses) return;

    m_pref->observePfMiss(address);

}

void GenericPrefetcher::observeMiss(const Address& address, const AccessType& type)
{

    /**
     * Fill miss data.
     */
    MissData.LastRequestCycle = g_eventQueue_ptr->getTime();
    MissData.LastRequestAddr  = 0;
    MissData.DataAddr = address.getAddress();
    MissData.SequenceNumber = 0;
    MissData.m_accesstype = type;
    MissData.hit = false;
    MissData.LastRequestPrefetch = false;

    if (!misses) return;

    m_pref->observeMiss(address,type);

}

void GenericPrefetcher::observeAccess(const Address& address, const AccessType& type)
{

   // Fill AccessData.

    AccessData.LastRequestCycle = g_eventQueue_ptr->getTime();
    AccessData.LastRequestAddr  = 0;
    AccessData.DataAddr = address.getAddress();
    AccessData.SequenceNumber = 0;
    AccessData.m_accesstype = type;
    AccessData.hit = false;
    AccessData.LastRequestPrefetch = false;

    if (!misses) return;

    m_pref->observeAccess(address,type);

}


void GenericPrefetcher::observePrefetchCallback(const Address& address)
{

    m_pref->observePrefetchCallback(address);

}

void  GenericPrefetcher::wakeup()
{

    /* Wakeup every 2 cycles to check for prefetch events to issue and perform housekeeping */
    g_eventQueue_ptr->scheduleEvent(this, 2);
    m_pref->wakeup();
    /*
     *  Call every cycle or don't. Depends on the prefetcher.
     *
    */
}
