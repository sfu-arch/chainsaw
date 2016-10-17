/*
 * simics/devs/mii.h
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

#ifndef _SIMICS_DEVS_MII_H
#define _SIMICS_DEVS_MII_H

#define MII_INTERFACE "mii"

typedef struct {
        int    (*serial_access)(conf_object_t *obj, int data_in, int clock);
        uint16 (*read_register)(conf_object_t *obj, int index);
        void   (*write_register)(conf_object_t *obj, int index, uint16 value);
} mii_interface_t;

typedef void (*mii_register_watch_callback_t)(void *parameter,
                                              uint16 old_value,
                                              uint16 new_value);

/* interface exported by the physical layer device */
#define MII_MANAGEMENT_INTERFACE "mii-management"
typedef struct {
        int    (*serial_access)(conf_object_t *obj, int data_in, int clock);
        uint16 (*read_register)(conf_object_t *obj, int phy, int reg);
        void   (*write_register)(conf_object_t *obj, int phy, int reg,
                                 uint16 value);
#if !defined(GULP)
        void   (*watch_register)(conf_object_t *obj, int phy, int reg,
                                 mii_register_watch_callback_t callback,
                                 void *parameter);
#endif
} mii_management_interface_t;

#endif /* _SIMICS_DEVS_MII_H */
