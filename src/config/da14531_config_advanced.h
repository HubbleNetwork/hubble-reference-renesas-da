/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DA14531_CONFIG_ADVANCED_H_
#define _DA14531_CONFIG_ADVANCED_H_

/*
 * arch.h only clears DO_NOT_RETAIN_ALL_RAM_BLOCKS for the 531 when blocks 1
 * and 2 are both retained. Without this the SDK powers RAM down in extended sleep.
 */
#define CFG_RETAIN_RAM_2_BLOCK

/* This config required for DA14531. */
#define CFG_LLM_RESOLVING_LIST_MAX  LLM_RESOLVING_LIST_MAX

#include "da1458x_config_advanced.h"

#endif /* _DA14531_CONFIG_ADVANCED_H_ */
