/*
 * simics/devs/reset.h
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

#ifndef _SIMICS_DEVS_RESET_H
#define _SIMICS_DEVS_RESET_H

/* <add id="reset_interface_t">
   The <i>reset</i> functions gets invoked whenever a caller decides
   that the device should reset itself. The argument <i>hard</i> is
   nonzero if the device should perform a hard reset.  Otherwise a
   soft reset should be done.  

   <insert-until text="// ADD INTERFACE reset_interface"/>
   </add>
*/
typedef struct {
        void (*reset)(conf_object_t *, int hard);
} reset_interface_t;

#define RESET_INTERFACE "reset"
// ADD INTERFACE reset_interface

#endif /* _SIMICS_DEVS_RESET_H */
