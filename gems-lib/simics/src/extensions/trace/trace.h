/*
  trace.h

  Copyright (C) 1998-2004 Virtutech AB, All Rights Reserved

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

typedef enum { 
        TR_Reserved = 0, TR_Data = 1, TR_Instruction = 2, TR_Exception = 3, 
        TR_Execute = 4, TR_Reserved_2, TR_Reserved_3, TR_Reserved_4,
        TR_Reserved_5, TR_Reserved_6, TR_Reserved_7, TR_Reserved_8,
        TR_Reserved_9, TR_Reserved_10, TR_Reserved_11, TR_Reserved_12
} trace_type_t;

typedef enum {
        TA_generic, TA_x86, TA_ia64, TA_v9
} trace_arch_t;

typedef struct {
        unsigned arch:2;
        unsigned trace_type:4;	        /* data, instruction, or exception (trace_type_t) */
        int      cpu_no:16;		/* processor number (0 and up) */
        uint32   size;
        unsigned read_or_write:1;	/* read_or_write_t */

        /* x86 */
        unsigned linear_access:1;       /* if linear access */
        unsigned seg_reg:3;             /* 0-5, with ES=0, CS=1, SS=2, DS=3, FS=4, GS=5 */
        linear_address_t la;
        x86_memory_type_t memory_type;

        /* x86_access_type_t or sparc_access_type_t */
        int access_type;

        /* virtual and physical address of effective address */
        logical_address_t va;
        physical_address_t pa;

        int atomic;

        union {
                uint64 data;            /* if TR_Date */
                uint8  text[16];        /* if TR_Instruction or TR_Execute
                                           (large enough for any target) */
                int exception;		/* if TR_Exception */
        } value;

        cycles_t timestamp;             /* Delta coded */
} trace_entry_t;

/* ADD INTERFACE trace_consume_interface */
typedef struct {
        void (*consume)(conf_object_t *obj, trace_entry_t *entry);
} trace_consume_interface_t;

#define TRACE_CONSUME_INTERFACE "trace_consume"
