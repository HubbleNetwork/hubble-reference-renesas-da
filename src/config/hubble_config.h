/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HUBBLE_CONFIG_H_
#define _HUBBLE_CONFIG_H_

#define CONFIG_HUBBLE_BLE_NETWORK 1

/* 128-bit only on this part. See hubble/hubble_crypto_renesas.c. */
#define CONFIG_HUBBLE_NETWORK_KEY_128 1

#ifdef CONFIG_HUBBLE_NETWORK_KEY_256
#error "CONFIG_HUBBLE_NETWORK_KEY_256 is not supported on DA145xx: the crypto is 128-bit only"
#endif

/* Backend supplied by this application: hubble/hubble_crypto_renesas.c. */
#define CONFIG_HUBBLE_NETWORK_CRYPTO_CUSTOM 1

#define CONFIG_HUBBLE_COUNTER_SOURCE_UNIX_TIME 1

#define CONFIG_HUBBLE_EID_ROTATION_PERIOD_SEC 86400

#define CONFIG_HUBBLE_NETWORK_SECURITY_ENFORCE_NONCE_CHECK 1

#define CONFIG_HUBBLE_LOG_LEVEL_OFF 1

#endif /* _HUBBLE_CONFIG_H_ */
