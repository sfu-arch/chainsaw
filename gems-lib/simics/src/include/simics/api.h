/*
 * simics/api.h
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

#ifndef _SIMICS_API_H
#define _SIMICS_API_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <simics/module-host-config.h>
#include <simics/global.h>

#include <simics/core/types.h>
#include <simics/core/breakpoints.h>
#include <simics/core/callbacks.h>
#include <simics/core/configuration.h>
#include <simics/core/control.h>
#include <simics/core/event.h>
#include <simics/core/front.h>
#include <simics/core/profiling.h>
#include <simics/core/statistics.h>
#include <simics/core/memory.h>
#include <simics/core/device.h>
#include <simics/core/processor.h>
#include <simics/core/decoder.h>
#include <simics/core/hindsight.h>
#include <simics/core/tick.h>
#include <simics/core/disassemble.h>

#if defined(ADD_ARCH_TO_API) || defined(GULP)
/* Only for 2.2 API and older */
#include <simics/arch/alpha.h>
#include <simics/arch/x86.h>
#include <simics/arch/ppc.h>
#include <simics/arch/arm.h>
#include <simics/arch/mips.h>
#include <simics/arch/ia64.h>
#include <simics/arch/sparc.h>
#endif /* ADD_ARCH_TO_API */

#include <simics/core/obsolete.h>

#if defined(__cplusplus)
}
#endif

#endif /* _SIMICS_API_H */
