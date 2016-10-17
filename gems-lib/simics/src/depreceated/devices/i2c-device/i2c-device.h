/*
  i2c-device.h

  Copyright (C) 2002-2003 Virtutech AB, All Rights Reserved

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

#ifndef I2C_DEVICE_H
#define I2C_DEVICE_H

#include <simics/devs/i2c.h>

enum i2c_pin_state {
        I2C_Pin_Idle,
        I2C_Pin_Address,
        I2C_Pin_Address_Ack,
        I2C_Pin_Read,
        I2C_Pin_Read_Ack,
        I2C_Pin_Write,
        I2C_Pin_Write_Ack
};

typedef struct i2c_device {
        log_object_t *log;

        enum i2c_pin_state state;

        unsigned clock_out : 1, data_out : 1, remote_data:1;
        uint8 data;
        int bit;

        conf_object_t *i2c_bus;
        i2c_bus_interface_t i2c_bus_iface;
} i2c_device_t;

void i2c_device_init(log_object_t *log, i2c_device_t *i2c_device);
void register_i2c_device_attributes(conf_class_t *i2c_class);

void i2c_write_clock(i2c_device_t *dev, int state);
void i2c_write_data(i2c_device_t *dev, int state);

int i2c_read_data(i2c_device_t *dev);
int i2c_read_clock(i2c_device_t *dev);

/* implemented by the user */
i2c_device_t *get_i2c_device(conf_object_t *obj);

#endif /* I2C_DEVICE_H */
