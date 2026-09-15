/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DA14535_CONFIG_BASIC_H_
#define _DA14535_CONFIG_BASIC_H_

/* Application features to be included by this project. */

#define CFG_BLE
#define CFG_APP

/* Heap size is set by this; cannot be 0. */
#define CFG_MAX_CONNECTIONS (1)

/*
 * Watchdog on. With CFG_DEVELOPMENT_DEBUG, a timeout halts for a debugger
 * instead of resetting. A production image wants CFG_DEVELOPMENT_DEBUG
 * undefined and CFG_WDG_TRIGGER_HW_RESET_IN_PRODUCTION_MODE defined.
 */
#define CFG_WDOG
#define CFG_DEVELOPMENT_DEBUG

#include "da1458x_stack_config.h"

#endif /* _DA14535_CONFIG_BASIC_H_ */
