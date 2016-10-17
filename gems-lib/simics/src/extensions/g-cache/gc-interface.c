/*
  gc-interface.c

  Copyright (C) 2003-2004 Virtutech AB, All Rights Reserved

  This program is a component ("Component") of Virtutech Simics and is
  being distributed under Section 1(a)(iv) of the Virtutech Simics
  Software License Agreement (the "Agreement").  You should have
  received a copy of the Agreement with this Component; if not, please
  write to Virtutech AB, Norrtullsgatan 15, 1tr, SE-113 27 STOCKHOLM,
  Sweden for a copy of the Agreement prior to using this Component.

  By using this Component, you agree to be bound by all of the terms of
  the Agreement.  If you do not agree to the terms of the Agreement, you
  may not use, copy or otherwise access the Component or any derivatives
  thereof.  You may create and use derivative works of this Component
  pursuant to the terms the Agreement provided that any such derivative
  works may only be used in conjunction with and as a part of Virtutech
  Simics for use by an authorized licensee of Virtutech.

  THIS COMPONENT AND ANY DERIVATIVES THEREOF ARE PROVIDED ON AN "AS IS"
  BASIS.  VIRTUTECH MAKES NO WARRANTIES WITH RESPECT TO THE COMPONENT OR
  ANY DERIVATIVES THEREOF AND DISCLAIMS ALL IMPLIED WARRANTIES,
  INCLUDING, WITHOUT LIMITATION, WARRANTIES OF MERCHANTABILITY AND
  FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <simics/api.h>
#include <simics/alloc.h>
#include <simics/utils.h>

#include "gc-common.h"
#include "gc.h"

/*
  Return 1 is the transaction is uncacheable.
*/
int
is_uncacheable(generic_cache_t *gc, generic_transaction_t *mem_op, 
               map_list_t *map, log_object_t *o)
{
        switch(mem_op->ini_type & 0xFF00) {

#ifdef GC_DEBUG
                /* this shouldn't happen */
        case Sim_Initiator_Illegal:
                SIM_log_error(o, GC_Log_Cache,
                              "is_uncacheable: Illegal initiator "
                              "type for memory transaction");
                break;
#endif

        case Sim_Initiator_CPU_V9: 
        {
                v9_memory_transaction_t *mt = 
                        (v9_memory_transaction_t *) mem_op;
                if (gc->config.virtual_tag)
                        return !mt->cache_virtual;
                else
                        return !mt->cache_physical;
                break;
        }

        case Sim_Initiator_CPU_PPC:
        {
                ppc_memory_transaction_t *mt =
                        (ppc_memory_transaction_t *)mem_op;
                return mt->wimg & 0x4; /* the I (cache inhibit) bit */
        }

        case Sim_Initiator_CPU_X86:
        {
                x86_memory_transaction_t *mt =
                        (x86_memory_transaction_t *) mem_op;
                if (mt->effective_type == X86_Write_Back)
                        return 0;
                else
                        return 1;
                break;
        }

        case Sim_Initiator_CPU_MIPS: 
        {
                mips_memory_transaction_t *mt = 
                        (mips_memory_transaction_t *) mem_op;

                switch (mt->s.ini_type) {
                case Sim_Initiator_CPU_MIPS_RM7000:
                        switch (mt->cache_coherency) {
                        case 2: /* Uncached, blocking. */
                        case 6: /* Uncached, non-blocking. */
                                return 1;
                        case 0: /* Writethrough without write-allocate,
                                   non-blocking. */
                        case 1: /* Writethrough with write-allocate,
                                   non-blocking. */
                        case 3: /* Writeback, non-blocking. */
                        case 7: /* Bypass, non-blocking. */
                                return 0;
                        }
                        break;
                case Sim_Initiator_CPU_MIPS_E9000:
                        switch (mt->cache_coherency) {
                        case 2: /* Uncached, blocking. */
                        case 6: /* Uncached, non-blocking. */
                                return 1;
                        case 0: /* Writethrough without write-allocate,
                                   non-blocking. */
                        case 1: /* Writethrough with write-allocate,
                                   non-blocking. */
                        case 3: /* Writeback, non-blocking. */
                        case 7: /* Bypass, non-blocking. */
                        case 4: /* Coherent writeback, exclusive allocate */
                        case 5: /* Coherent writeback */
                                return 0;
                        }
                        break;
                default:
                        switch (mt->cache_coherency) {
                        case 2: /* Uncached. */
                                return 1;
                        case 3: /* Cacheable. */
                                return 0;
                        }
                        break;
                }
                SIM_log_error(o, GC_Log_Cache,
                              "Unknown cache coherency "
                              "value: %d", mt->cache_coherency);
                return 1;
        }

        case Sim_Initiator_Cache:
        {
                return 0;
        }

        default:
                if ((mem_op->ini_type & 0xF000) == Sim_Initiator_CPU)
                        return 0; /* unknown cpu transaction are cacheable */
                else
                        return 1; /* device transactions are uncacheable */
        }

        /* error is uncacheable */
        return 1;
}
