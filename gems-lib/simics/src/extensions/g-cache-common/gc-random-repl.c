/*
  gc-random-repl.c - random replacement implementation for all caches

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

/* cache includes */
#include "gc-common.h"
#include "gc.h"

typedef struct rand_data {
        uint32 seed;
} rand_data_t;

static int
rand_val(uint32 *seed_addr)
{
        uint32 tmp;
        tmp = (*seed_addr + 13) * 61147;
        *seed_addr = tmp;
        return (tmp >> 16);
}

/* return a random cache line in a set */
#define GC_RAND_REPLACE(gc, data) \
        (rand_val(&(data)->seed) % (gc)->config.assoc)

static const char *
rand_get_name(void)
{
        return "random";
}

static void
rand_update_repl(void *data, generic_cache_t *gc, generic_transaction_t *gt, 
                 int line_num)
{
}

static int
rand_get_line(void *data, generic_cache_t *gc, generic_transaction_t *gt)
{
        rand_data_t *d = (rand_data_t *) data;

        int line_num =  GC_INDEX(gc, gt)
                + (GC_RAND_REPLACE(gc, d) * gc->config.next_assoc);
        SIM_log_info(3, &gc->log, GC_Log_Repl,
                     "get_line::random: got line %d", line_num);
        return line_num;
}

static void *
rand_new_instance(generic_cache_t *gc)
{
        rand_data_t *d;

        d = MM_ZALLOC(1, rand_data_t);
        return d;
}

static void
rand_update_config(void *data, generic_cache_t *gc)
{
}

static set_error_t                                              
set_config_seed(void *dont_care, conf_object_t *obj,       
                attr_value_t *val, attr_value_t *idx)      
{                                                               
        generic_cache_t *gc = (generic_cache_t *) obj;          

        if (strcmp(gc->config.repl_fun.get_name(), rand_get_name()) == 0) {
                rand_data_t *d = (rand_data_t *) gc->config.repl_data;
                d->seed = val->u.integer;
                return Sim_Set_Ok;                   
        }
        else {
                SIM_log_info(4, &gc->log, GC_Log_Repl,
                             "Random replacement policy is not in use.");
                return Sim_Set_Illegal_Value;
        }
}

static attr_value_t                                         
get_config_seed(void *dont_care, conf_object_t *obj,       
                attr_value_t *idx)                    
{                                                          
        generic_cache_t *gc = (generic_cache_t *) obj;     
        attr_value_t ret;                                  

        if (strcmp(gc->config.repl_fun.get_name(), rand_get_name()) == 0) {
                rand_data_t *d = (rand_data_t *) gc->config.repl_data;
                ret.kind = Sim_Val_Integer;                        
                ret.u.integer = d->seed;
        }
        else {
                SIM_log_info(4, &gc->log, GC_Log_Repl,
                             "Random replacement policy is not in use.");
                ret.kind = Sim_Val_Invalid;
        }

        return ret;                                        
}

repl_interface_t *
init_rand_repl(conf_class_t *gc_class)
{
        repl_interface_t *rand_i;

        /* config_seed */
        SIM_register_typed_attribute(
                gc_class, "config_seed",
                get_config_seed, 0,
                set_config_seed, 0,
                Sim_Attr_Optional,
                "i", NULL,
                "Seed for random replacement policy, default is 0).");

        rand_i = MM_ZALLOC(1, repl_interface_t);

        rand_i->new_instance = rand_new_instance;
        rand_i->update_config = rand_update_config;
        rand_i->get_name = rand_get_name;
        rand_i->update_repl = rand_update_repl;
        rand_i->get_line = rand_get_line;

        return rand_i;
}
