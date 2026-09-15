/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DA14535_CONFIG_ADVANCED_H_
#define _DA14535_CONFIG_ADVANCED_H_

/* Memory map and radio configuration. */

/*
 * Retention memory: the RAM that survives extended sleep. The uptime wrap
 * counter (hubble/hubble_port.c) and the ChaCha state live here.
 *
 * The toolchain conditional is the SDK's own; clang and GCC lay retained data
 * out differently.
 */
#if defined(__clang__)
#define CFG_RET_DATA_SIZE           (2400)
#else
#define CFG_RET_DATA_SIZE           (2200)
#endif
#define CFG_RET_DATA_UNINIT_SIZE    (0)

#define CFG_AUTO_DETECT_NON_RET_HEAP

/* Keeps RAM block 1 powered during sleep, which is what makes the above retain. */
#define CFG_RETAIN_RAM_1_BLOCK

#define CFG_MAX_TX_PACKET_LENGTH    (31)
#define CFG_MAX_RX_PACKET_LENGTH    (31)

/* Use low-power oscillator clock */
#define CFG_LP_CLK                  LP_CLK_RCX20
#define CFG_NVDS_TAG_LPCLK_DRIFT    DRIFT_50PPM

/* Ceilings on the stack's own sleep, not the advertising interval. */
#define CFG_MAX_SLEEP_DURATION_PERIODIC_WAKEUP_MS       500
#define CFG_MAX_SLEEP_DURATION_EXTERNAL_WAKEUP_MS       600000

/* Default BT address. Overwritten by NRPA use. */
#define CFG_NVDS_TAG_BD_ADDRESS {0}

/* Channel assessment: avoid congested channels. */
#define CFG_NVDS_TAG_BLE_CA_TIMER_DUR       (500)
#define CFG_NVDS_TAG_BLE_CRA_TIMER_DUR      (8)
#define CFG_NVDS_TAG_BLE_CA_MIN_RSSI        (-60)
#define CFG_NVDS_TAG_BLE_CA_NB_PKT          (20)
#define CFG_NVDS_TAG_BLE_CA_NB_BAD_PKT      (CFG_NVDS_TAG_BLE_CA_NB_PKT / 2)

/* ChaCha20 used for NRPA addresses. */
#define CFG_TRNG
#define CFG_USE_CHACHA20_RAND

/* Temperature compensation for radio calibration. */
#define CFG_AMB_TEMPERATURE

 /* Code is not stored in OTP. */
#define CFG_CODE_LOCATION_EXT

/* Quadec unused; disable to save power. */
#define CFG_DISABLE_QUADEC_ON_START_UP

/* Single-chip application: no UART HCI transport. */
#define CFG_USE_H4TL 0

/* Unused by a broadcaster, but the stack expects the symbol. */
#define CFG_BLE_DUPLICATE_FILTER_MAX 0

#include "da1458x_stack_config.h"

#endif /* _DA14535_CONFIG_ADVANCED_H_ */
