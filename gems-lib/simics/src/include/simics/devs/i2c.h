/*
 * simics/devs/i2c.h
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

#ifndef _SIMICS_DEVS_I2C_H
#define _SIMICS_DEVS_I2C_H

/* 

   Interfaces for the i2c architecture
   -----------------------------------

   The i2c bidirectional 2-wire bus and data transmission protocol was
   developed in the early 1980's by Philips semiconductors. The i2c is
   an acronym for inter-IC (iic), and this name literally explains the
   purpose; to connect integrated circuits to a common bus.
   
   A device that sends data to the bus is defined as a transmitter,
   and a device receiving data from the bus is defined as a receiver.
   The device that controls the transfer is called a master, and
   devices that are controlled by the master are called slaves. This
   gives us a total of four different modes for device operations:

   - master/transmitter
   - master/receiver
   - slave/transmitter
   - slave/receiver

   Note that not all devices can operate as a bus master.

   The transfer must be initiated by the master, through a start (or
   repeat-start) condition. All data are transfered one byte at a
   time, and the first byte contains the slave address (with the least
   significant bit as a read/write flag).

*/

  
/* states of the i2c device (well, actually the idle state refers to the i2c
   _bus_). Names used in checkpointing. */
#define FOR_ALL_I2C_DEVICE_STATE(op)                    \
        op(I2C_idle,            "I2C Idle"),            \
        op(I2C_master_transmit, "I2C Master Transmit"), \
        op(I2C_master_receive,  "I2C Master Receive"),  \
        op(I2C_slave_transmit,  "I2C Slave Transmit"),  \
        op(I2C_slave_receive,   "I2C Slave Receive")

#define I2C_FIRST_ELEMENT(a,b) a
typedef enum i2c_device_state {
        FOR_ALL_I2C_DEVICE_STATE(I2C_FIRST_ELEMENT),
        I2C_max_states
} i2c_device_state_t;
#undef I2C_FIRST_ELEMENT

typedef enum i2c_device_flag {
   
        I2C_flag_exclusive,
        I2C_flag_shared
        /* more to follow in the future */
   
} i2c_device_flag_t;
   
/* interface implemented by all i2c buses */
typedef struct i2c_bus_interface {
   
        int (*start)(conf_object_t *i2c_bus, uint8 address);
        int (*stop)(conf_object_t *i2c_bus);
       
        uint8 (*read_data)(conf_object_t *i2c_bus);
        void (*write_data)(conf_object_t *i2c_bus, uint8 value);
       
        int (*register_device)(conf_object_t *i2c_bus, conf_object_t *device,
                               uint8 address, uint8 mask, i2c_device_flag_t flags);
        void (*unregister_device)(conf_object_t *i2c_bus, conf_object_t *device,
                                  uint8 address, uint8 mask);
       
} i2c_bus_interface_t;
   
/* interface implemented by all i2c devices */
typedef struct i2c_device_interface {
       
        int (*set_state)(conf_object_t *device, i2c_device_state_t state,
                        uint8 address);
       
        uint8 (*read_data)(conf_object_t *device);
        void (*write_data)(conf_object_t *device, uint8 value);
   
} i2c_device_interface_t;

#define I2C_BUS_INTERFACE "i2c-bus"
#define I2C_DEVICE_INTERFACE "i2c-device"
// ADD INTERFACE i2c_bus_interface
// ADD INTERFACE i2c_device_interface

#endif /* _SIMICS_DEVS_I2C_H */
