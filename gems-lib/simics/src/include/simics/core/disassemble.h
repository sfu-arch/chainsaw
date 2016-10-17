/*
 * simics/core/disassemble.h
 *
 * This file is part of Virtutech Simics
 *
 * Copyright (C) 1998-2005 Virtutech AB, All Rights Reserved
 *
 * The Virtutech Simics API is distributed under license. Please refer to
 * the 'LICENSE' file for details.
 *
 * For documentation on the Virtutech Simics API, please refer to the
 * Simics Reference Manual. Please report any difficulties you encounter
 * with this API through the Virtutech Simics website at www.simics.com
 *
 */

#ifndef _SIMICS_CORE_DISASSEMBLE_H
#define _SIMICS_CORE_DISASSEMBLE_H

/* <add-type id="disasm_instr_t"> </add-type> */
typedef struct {
        int start;          /* Where the instructions starts in the buffer */
        int length;         /* Length of instruction, or -1 if incomplete */
        const char *string; /* Disassembly string (static) */
} disasm_instr_t;

/* <add-type id="disassemble_interface_t">
   The disassemble interface can be used to disassemble code from a
   buffer in memory. It is typically used to disassemble code for the
   host architecture independent of the target architecture
   implemented in a particular version of Simics.

   <fun>init()</fun> is used to initialize a new disassemble
   session. You should provide a buffer in <tt>buff</tt>, the buffer
   length in bytes in <tt>buff_len</tt> and the base address for this
   chunk in <tt>address</tt>. The <tt>address</tt> parameter is used
   to calculate program counter relative offsets (for branches and
   other program counter relative constructs).

   <ndx>disasm_instr_t</ndx>
   <insert id="disasm_instr_t"/>

   <fun>next()</fun> returns a structure with the next disassembled
   instruction. Repeated use of <fun>next()</fun> will disassemble
   additional instructions.
   </add-type> */
typedef struct {
        /* Set up new block to disassemble */
        void (*init)(conf_object_t *obj, uint8 *buff,
                     int buff_len, uinteger_t address);
        /* Disassemble the next instruction */
        disasm_instr_t (*next)(conf_object_t *obj);
} disassemble_interface_t;

#endif /* _SIMICS_CORE_DISASSEMBLE_H */
