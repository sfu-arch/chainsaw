/*
 * simics/devs/recorder.h
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

#ifndef _SIMICS_DEVS_RECORDER_H
#define _SIMICS_DEVS_RECORDER_H

#define RECORDER_INTERFACE "recorder"

#if !defined(GULP)
typedef struct recorder recorder_t;

/*
 * dbuffer ownership is _not_ transfered. The callback must not modify the
 * dbuffer without cloning it first.
 */

typedef void (*recorder_input_handler_t)(conf_object_t *obj, dbuffer_t *data,
                                         uint32 uint_data);

typedef struct recorder_interface {
        recorder_t *(*attach)(conf_object_t *rec, conf_object_t *obj,
                              recorder_input_handler_t input_from_recorder);
        void (*detach)(recorder_t *r);
        void (*input)(recorder_t *r, dbuffer_t *data, uint32 uint_data);
} recorder_interface_t;
#endif /* !GULP */

#endif /* _SIMICS_DEVS_RECORDER_H */
