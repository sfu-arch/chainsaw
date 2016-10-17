/*
 * simics/core/statistics.h
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

#ifndef _SIMICS_CORE_STATISTICS_H
#define _SIMICS_CORE_STATISTICS_H

int SIM_add_mode_counter(conf_object_t *cpu, const char *desc);
integer_t SIM_get_mode_counter(conf_object_t *cpu, processor_mode_t mode,
                               int counter);
void SIM_set_mode_counter(conf_object_t *cpu, processor_mode_t mode,
                          int counter, integer_t value);
void SIM_inc_mode_counter(conf_object_t *cpu, processor_mode_t mode,
                          int counter);
void SIM_inc_cur_mode_counter(conf_object_t *cpu, int counter);

#endif /* _SIMICS_CORE_STATISTICS_H */
