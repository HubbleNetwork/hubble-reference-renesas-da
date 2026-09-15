/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USER_PERIPH_SETUP_H_
#define _USER_PERIPH_SETUP_H_

/*
 * Pinmux functions required by Renesas SDK
 */

#include "gpio.h"

void periph_init(void);
void GPIO_reservations(void);
void set_pad_functions(void);

#endif /* _USER_PERIPH_SETUP_H_ */
