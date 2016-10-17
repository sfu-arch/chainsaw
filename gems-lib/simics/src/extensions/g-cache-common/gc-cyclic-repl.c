/*
  gc-cyclic-repl.c - cyclic replacement implementation for all caches

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

#include "simics/api.h"
#include "simics/alloc.h"
#include "simics/utils.h"

/* cache includes */
#include "gc-common.h"
#include "gc.h"

typedef struct cycle_data {
        uint32 *next_line_in_set;
} cyclic_data_t;

static const char *
cyclic_get_name(void)
{
        return "cyclic";
}

static void
cyclic_update_repl(void *data, generic_cache_t *gc, generic_transaction_t *gt, 
                   int line_num)
{
}

static int
cyclic_get_line(void *data, generic_cache_t *gc, generic_transaction_t *gt)
{
        cyclic_data_t *d = (cyclic_data_t *) data;
        int index = GC_INDEX(gc, gt);
        int line_num =  d->next_line_in_set[index];

        /* increase to next line in set */
        d->next_line_in_set[index] = 
                ((line_num + gc->config.next_assoc) >= gc->config.line_number)
                ? index 
                : (line_num + gc->config.next_assoc);
        
        SIM_log_info(4, &gc->log, GC_Log_Repl, 
                     "cyclic::get_line: got line %d", line_num);
        return line_num;
}

static void *
cyclic_new_instance(generic_cache_t *gc)
{
        cyclic_data_t *d;
        int i;

        d = MM_MALLOC(1, cyclic_data_t);
        d->next_line_in_set = MM_MALLOC(gc->config.next_assoc, uint32);
        for (i=0; i<gc->config.next_assoc; i++)
                d->next_line_in_set[i] = i;
        return d;
}

static void
cyclic_update_config(void *data, generic_cache_t *gc) 
{
        cyclic_data_t *d = (cyclic_data_t *) data;
        int i;

        MM_FREE(d->next_line_in_set);
        d->next_line_in_set = MM_MALLOC(gc->config.next_assoc, uint32);
        for (i=0; i<gc->config.next_assoc; i++)
                d->next_line_in_set[i] = i;
}

static set_error_t                                              
set_next_line_in_set(void *dont_care, conf_object_t *obj,       
                      attr_value_t *val, attr_value_t *idx)      
{                                                               
        generic_cache_t *gc = (generic_cache_t *) obj;          

        if (strcmp(gc->config.repl_fun.get_name(), cyclic_get_name()) == 0) {
                cyclic_data_t *d = (cyclic_data_t *) gc->config.repl_data;
                int i;

                if (!idx || idx->kind != Sim_Val_Integer) {

                        if (val->u.list.size != gc->config.next_assoc) {
                                SIM_log_error(&gc->log, GC_Log_Repl,
                                              "generic-cache::next_line: "
                                              "this has cache doesn't have the "
                                              "right number of set for loading "
                                              "the cyclic values.");
                                return Sim_Set_Illegal_Value;
                        }

                        for (i=0; i<gc->config.next_assoc; i++)
                                d->next_line_in_set[i] = 
                                        val->u.list.vector[i].u.integer;
                }
                else {
                        d->next_line_in_set[idx->u.integer] = 
                                val->u.list.vector[idx->u.integer].u.integer;
                }

                return Sim_Set_Ok;                   
        }
        else {
                SIM_log_info(4, &gc->log, GC_Log_Repl,
                             "Cyclic replacement policy is not in use.");
                return Sim_Set_Illegal_Value;
        }
}

static attr_value_t                                         
get_next_line_in_set(void *dont_care, conf_object_t *obj,       
                      attr_value_t *idx)                    
{                                                          
        generic_cache_t *gc = (generic_cache_t *) obj;     
        attr_value_t ret;                                  

        if (strcmp(gc->config.repl_fun.get_name(), cyclic_get_name()) == 0) {
                cyclic_data_t *d = (cyclic_data_t *) gc->config.repl_data;
                int i;

                if (!idx || idx->kind != Sim_Val_Integer) {

                        ret.kind = Sim_Val_List;
                        ret.u.list.size = gc->config.next_assoc;
                        ret.u.list.vector = MM_MALLOC(ret.u.list.size,
                                                      attr_value_t);

                        for (i=0; i<gc->config.next_assoc; i++) {

                                ret.u.list.vector[i].kind = Sim_Val_Integer;
                                ret.u.list.vector[i].u.integer = 
                                        d->next_line_in_set[i];
                        }
                }
                else {
                        /* just one element */
                        ret.kind = Sim_Val_Integer;
                        ret.u.integer = d->next_line_in_set[idx->u.integer];
                }
        }
        else {
                SIM_log_info(2, &gc->log, GC_Log_Repl,
                             "Cyclic replacement policy is not in use.");
                ret.kind = Sim_Val_Invalid;
        }

        return ret;     
}

repl_interface_t *
init_cyclic_repl(conf_class_t *gc_class)
{
        repl_interface_t *cyclic_i;

        /* config_seed */
        SIM_register_typed_attribute(
                gc_class, "next_line_in_set",
                get_next_line_in_set, 0,
                set_next_line_in_set, 0,
                Sim_Attr_Optional | Sim_Attr_Integer_Indexed,
                "[i*]", "i",
                "Next line used for replacement in a given set.");

        cyclic_i = MM_ZALLOC(1, repl_interface_t);

        cyclic_i->new_instance = cyclic_new_instance;
        cyclic_i->update_config = cyclic_update_config;
        cyclic_i->get_name = cyclic_get_name;
        cyclic_i->update_repl = cyclic_update_repl;
        cyclic_i->get_line = cyclic_get_line;

        return cyclic_i;
}
