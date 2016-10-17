#ifndef ABSTRACT_PREFETCHER_H
#define ABSTRACT_PREFETCHER_H

#include "Global.h"
#include "AbstractChip.h"
#include "RubyConfig.h"
#include "Address.h"
#include "Types.h"

class AbstractPrefetcher {
public:

	virtual ~AbstractPrefetcher() {};

	/**
	 * [observePfHit Prefetch hit with another access]
	 * @param address [description]
	 */
	virtual void observePfHit(const Address& address) = 0;
	virtual void observePfMiss(const Address& address) = 0 ;

    /**
	* Observe a memory miss from the cache.
    *
    * @param address   The physical address that missed out of the cache.
    */
    virtual void observeMiss(const Address& address, const AccessType& type) = 0;

    /**
     * Protocol calls in to say things about the access
     * @param address [description]
     * @param type    [description]
     */
    virtual void observeAccess(const Address& address, const AccessType& type) = 0;


    /**
     * Protocol calls in to inform that prefetch has finished
     * @param address [description]
     */
    virtual void observePrefetchCallback(const Address& address) = 0;

    virtual void wakeup() = 0;

    virtual void print() const = 0;
  
private:
 
};

#endif