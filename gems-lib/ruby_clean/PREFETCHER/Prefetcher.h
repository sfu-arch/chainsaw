/*
 * Copyright (c) 1999-2012 Mark D. Hill and David A. Wood
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef STRIDE_PREFETCHER_H
#define STRIDE_PREFETCHER_H

// Implements Power 4 like prefetching

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

class GenericPrefetcher;

#define MAX_PF_INFLIGHT 8

class PrefetchEntry 
{
    public:
        /// constructor
        PrefetchEntry()
        {
            // default: 1 cache-line stride
            m_stride   = RubyConfig::dataBlockBytes();
            m_use_time = Time(0);
            m_is_valid = false;
        }

        //! The base address for the stream prefetch
        Address m_address;

        //! stride distance to get next address from
        int m_stride;

        //! the last time that any prefetched request was used
        Time m_use_time;

        //! valid bit for each stream
        bool m_is_valid;

        //! L1D prefetches loads and stores
        AccessType m_type;

        //! Bitset for tracking prefetches for which addresses have been
        //! issued, which ones have completed.
        std::bitset<MAX_PF_INFLIGHT> requestIssued;
        std::bitset<MAX_PF_INFLIGHT> requestCompleted;
};

class StridePrefetcher : public AbstractPrefetcher
{
    public:
        StridePrefetcher(AbstractChip*,GenericPrefetcher* g_pref_ptr);
        ~StridePrefetcher();

        void issueNextPrefetch(const Address &address, PrefetchEntry *stream);
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


       /**
        * Protocol calls in to inform that prefetch has finished
        * @param address [description]
        */
        void observePrefetchCallback(const Address& address);


        void wakeup() {};
        /**
         * Print out some statistics
         */
        void print() const;
        
        void regStats();

    private:
        /**
         * Returns an unused stream buffer (or if all are used, returns the
         * least recently used (accessed) stream buffer).
         * @return  The index of the least recently used stream buffer.
         */
        int getLRUindex(void);

        //! clear a non-unit stride prefetcher entry
        void clearNonunitEntry(int index);

        //! allocate a new stream buffer at a specific index
        void initializeStream(const Address& address, int stride,
            int index, const AccessType& type);

        //! get pointer to the matching stream entry, returns NULL if not found
        //! index holds the multiple of the stride this address is.
        PrefetchEntry* getPrefetchEntry(const Address &address,
            int &index);

        /// access a unit stride filter to determine if there is a hit
        bool accessUnitFilter(std::vector<Address>& filter_table,
             int *hit_table, int &index, const Address &address,
            int stride, bool &alloc);

        /// access a unit stride filter to determine if there is a hit
        bool accessNonunitFilter(const Address& address, int *stride,
            bool &alloc);



        // Data Members (m_ prefix)

        AbstractChip* m_chip_ptr;

        MachineType m_machType;

        GenericPrefetcher* m_pref_ptr;

        MessageBuffer* PrefetchQ;

        int m_version;

        //! number of prefetch streams available
        int m_num_streams;
        //! an array of the active prefetch streams
        std::vector<PrefetchEntry> m_array;

        //! number of misses I must see before allocating a stream
        int m_train_misses;
        //! number of initial prefetches to startup a stream
        int m_num_startup_pfs;
        //! number of stride filters
        int m_num_unit_filters;
        //! number of non-stride filters
        int m_num_nonunit_filters;

        /// a unit stride filter array: helps reduce BW requirement of
        /// prefetching
        std::vector<Address> m_unit_filter;
        /// a round robin pointer into the unit filter group
        int m_unit_filter_index;
        //! An array used to count the of times particular filter entries
        //! have been hit
        int *m_unit_filter_hit;

        //! a negative nit stride filter array: helps reduce BW requirement
        //! of prefetching
        std::vector<Address> m_negative_filter;
        /// a round robin pointer into the negative filter group
        int m_negative_filter_index;
        /// An array used to count the of times particular filter entries
        /// have been hit
        int *m_negative_filter_hit;

        /// a non-unit stride filter array: helps reduce BW requirement of
        /// prefetching
        std::vector<Address> m_nonunit_filter;
        /// An array of strides (in # of cache lines) for the filter entries
        int *m_nonunit_stride;
        /// An array used to count the of times particular filter entries
        /// have been hit
        int *m_nonunit_hit;
        /// a round robin pointer into the unit filter group
        int m_nonunit_index;

        /// Used for allowing prefetches across pages.
        bool m_prefetch_cross_pages;


        //! Count of accesses to the prefetcher
        int numMissObserved;
        //! Count of prefetch streams allocated
        int numAllocatedStreams;
        //! Count of prefetch requests made
        int numPrefetchRequested;
        //! Count of prefetch requests accepted
        int numPrefetchAccepted;
        //! Count of prefetches dropped
        int numDroppedPrefetches;
        //! Count of successful prefetches
        int numHits;
        //! Count of partial successful prefetches
        int numPartialHits;
        //! Count of pages crossed
        int numPagesCrossed;
        //! Count of misses incurred for blocks that were prefetched
        int numMissedPrefetchedBlocks;
};

#endif // STRIDE_PREFETCHER_H
