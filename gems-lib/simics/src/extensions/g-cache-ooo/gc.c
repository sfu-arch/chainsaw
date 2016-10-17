/*
  gc.c - g-cache-ooo class

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

/* include the generic code */
#include "gc-specialize.c"

/*
  Information for class registering
*/
static conf_class_t *gc_class;
static class_data_t gc_data;
static timing_model_interface_t gc_ifc;

DLL_EXPORT void
init_local(void)
{
        gc_data.new_instance = gc_new_instance;
        gc_data.description =
                "g-cache-ooo is an out-of-order cache model with the following "
                "features: "
                "configurable number of lines, line size, associativity; "
                "indexing/tagging on physical/virtual addresses; "
                "configurable policies for write-allocate and write-back; "
                "random, cyclic and lru replacement policies; "
                "several caches can be connected in chain; "
                "a single cache can be connected to several processors; "
                "multiple out-of-order transactions support.";
                
        if (!(gc_class = SIM_register_class("g-cache-ooo", 
                                            &gc_data))) {
                pr_err("Could not create g-cache-ooo class\n");
        } else {

                /* register the attributes */
                gc_common_register(gc_class);
                gc_register(gc_class);

                /* register the timing model interface */
                gc_ifc.operate = gc_operate;
                SIM_register_interface(gc_class, "timing-model", &gc_ifc);

                /* add the possiblereplacement policies */
                add_repl_policy(init_rand_repl(gc_class));
                add_repl_policy(init_lru_repl(gc_class));

                /* register the log groups */
                SIM_log_register_groups(gc_class, gc_log_groups);
        }

        /* register the other classes */
        ts_init_local();
        sort_init_local();
}
