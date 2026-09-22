/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * The basic configuration for both parts. CMake force-includes this into every
 * translation unit.
 *
 * The da1458x_ prefix is the SDK's own family-wide name for this file, not a
 * reference to the older DA1458x parts: every SDK6 example carries a
 * da1458x_config_basic.h, and the DA14531 linker script template includes it by
 * that name. The DA14535 template asks for da14535_config_basic.h instead, so
 * there is a one-line shim under that name beside this file.
 */

#ifndef _DA1458X_CONFIG_BASIC_H_
#define _DA1458X_CONFIG_BASIC_H_

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

#endif /* _DA1458X_CONFIG_BASIC_H_ */
