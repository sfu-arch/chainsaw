/*
  gc-common.c - common functions (all versions)

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

const char *gc_log_groups[10] = {"Read Hit Path", 
                                 "Read Miss Path", 
                                 "Write Hit Path", 
                                 "Write Miss Path", 
                                 "Misc. Cache Activites", 
                                 "Replacement Policies", 
                                 "MESI Protocol", 
                                 "Attributes",
                                 "Outstanding Transactions",
                                 NULL};

static VECT(repl_interface_t *) repl_policies = VNULL;

/* register a new replacement policy */
void
add_repl_policy(repl_interface_t *nw)
{
        VADD(repl_policies, nw);
}

uint32
ln2(uint32 new_value)
{
        int i;
        for (i = 0; i < 32; i++) {
                new_value >>= 1;
                if (new_value == 0)
                        break;
        }
        return i;
}

/* called when a new cache is created */
conf_object_t *
gc_new_instance(parse_object_t *pa)
{
        generic_cache_t *gc = MM_ZALLOC(1, generic_cache_t);

        SIM_log_constructor(&gc->log, pa);

        /* set default values in the cache */
        gc_init_cache(gc);

        return (conf_object_t *) gc;
}

void
gc_finalize_instance(conf_object_t *cache)
{
        generic_cache_t *gc = (generic_cache_t *) cache;
        update_precomputed_values(gc);
}


void
gc_set_config_line_number(generic_cache_t *gc, int line_number)
{
        gc->config.line_number = line_number;

        /* re-allocate the lines */
        MM_FREE(gc->lines);
        gc->lines = MM_ZALLOC(gc->config.line_number, cache_line_t);
        
        update_precomputed_values(gc);
}

int
gc_set_config_repl(generic_cache_t *gc, const char *repl)
{
        repl_interface_t *ri;
        int i = 0;

        if (!VLEN(repl_policies)) {
                SIM_log_error(&gc->log, GC_Log_Repl, 
                              "Cache has no replacement policies registered.");
                return -1;
        }
        
        ri = VGET(repl_policies, i);
        while (ri != NULL) {
                if (strcmp(repl, ri->get_name()) == 0) {
                        MM_FREE(gc->config.repl_data);
                        memcpy(&gc->config.repl_fun, ri, sizeof(*ri));
                        gc->config.repl_data = 
                                gc->config.repl_fun.new_instance(gc);
                        gc->config.repl_fun.update_config(
                                gc->config.repl_data, gc);
                        return 0;
                }
                i++;
                ri = VGET(repl_policies, i);
        }

        SIM_log_info(1, &gc->log, GC_Log_Repl,
                     "replacement: possible values are :");

        i = 0;
        ri = VGET(repl_policies, i);
        while (ri != NULL) {
                SIM_log_info(1, &gc->log, GC_Log_Repl, "   %s",
                             ri->get_name());
                i++;
                ri = VGET(repl_policies, i);
        }
        return -1;
}
