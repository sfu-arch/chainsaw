/*
 * simics/util/genrand.h
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
 * MT19937 random number generation.
 *
 */

#ifndef _SIMICS_UTIL_GENRAND_H
#define _SIMICS_UTIL_GENRAND_H

typedef struct rand_state rand_state_t;

rand_state_t *genrand_init(uint32 s);
rand_state_t *genrand_init_array(uint32 init_key[], unsigned key_length);
rand_state_t *genrand_init_attr(attr_value_t *val);
attr_value_t genrand_state_to_attr(rand_state_t *rs);

uint32 genrand_uint32(rand_state_t *rs);
uint64 genrand_uint64(rand_state_t *rs);
double genrand_double(rand_state_t *rs);

#endif /* _SIMICS_UTIL_GENRAND_H */
